#include "display.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// VSPI's own IO_MUX pins for clock/data/CS, so 40 MHz is fine (checked on
// the breadboard with ../esp32-hw-checks).
#define SPI_HOST_ID SPI3_HOST
#define PIN_SCLK    18 // module "SCL"
#define PIN_MOSI    23 // module "SDA"
#define PIN_RST     17
#define PIN_DC      16
#define PIN_CS      5
#define PIN_BLK     19
#define PCLK_HZ     (40 * 1000 * 1000)
#define Y_GAP       20 // controller RAM is 240x320, the panel shows rows 20..299
// The panel's picture is upside down with the pin header at the bottom;
// 0 for the header at the top.
#define FLIP_180 1

#define STRIP_ROWS 20 // 9.6 KB DMA buffer
#define STRIPS     (CANVAS_H / STRIP_ROWS)

static esp_lcd_panel_handle_t s_panel;
static SemaphoreHandle_t s_sent;
static uint16_t *s_strip;
static uint32_t s_shown[STRIPS]; // hash of each strip as last sent
static bool s_shown_valid;

static bool IRAM_ATTR on_sent(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *edata, void *ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_sent, &woken);
    return woken == pdTRUE;
}

void display_backlight(int percent)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (1023 * percent) / 100);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

static esp_err_t backlight_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer);
    if (err != ESP_OK) return err;
    ledc_channel_config_t chan = {
        .gpio_num = PIN_BLK,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0, // dark until the first frame is drawn
    };
    return ledc_channel_config(&chan);
}

esp_err_t display_init(void)
{
    esp_err_t err = backlight_init();
    if (err != ESP_OK) return err;

    spi_bus_config_t bus = {
        .sclk_io_num = PIN_SCLK,
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CANVAS_W * STRIP_ROWS * sizeof(uint16_t),
    };
    if ((err = spi_bus_initialize(SPI_HOST_ID, &bus, SPI_DMA_CH_AUTO)) != ESP_OK) return err;

    s_sent = xSemaphoreCreateBinary();
    s_strip = heap_caps_malloc(CANVAS_W * STRIP_ROWS * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (!s_sent || !s_strip) return ESP_ERR_NO_MEM;

    esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = PIN_CS,
        .dc_gpio_num = PIN_DC,
        .spi_mode = 0,
        .pclk_hz = PCLK_HZ,
        .trans_queue_depth = 4,
        .on_color_trans_done = on_sent,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    esp_lcd_panel_io_handle_t io;
    if ((err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI_HOST_ID, &io_cfg, &io)) != ESP_OK)
        return err;

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };
    if ((err = esp_lcd_new_panel_st7789(io, &panel_cfg, &s_panel)) != ESP_OK) return err;
    if ((err = esp_lcd_panel_reset(s_panel)) != ESP_OK) return err;
    if ((err = esp_lcd_panel_init(s_panel)) != ESP_OK) return err;
    if ((err = esp_lcd_panel_invert_color(s_panel, true)) != ESP_OK) return err; // IPS panels need INVON
    // Flipping both axes of the 320-row RAM keeps the 280 visible rows at
    // the same gap, 20 either side.
    if ((err = esp_lcd_panel_mirror(s_panel, FLIP_180, FLIP_180)) != ESP_OK) return err;
    if ((err = esp_lcd_panel_set_gap(s_panel, 0, Y_GAP)) != ESP_OK) return err;
    return esp_lcd_panel_disp_on_off(s_panel, true);
}

// FNV-1a: cheap enough to run over every strip of every frame.
static uint32_t hash(const uint16_t *px, int n)
{
    uint32_t h = 2166136261u;
    for (int i = 0; i < n; i++) {
        h = (h ^ (px[i] & 0xFF)) * 16777619u;
        h = (h ^ (px[i] >> 8)) * 16777619u;
    }
    return h;
}

esp_err_t display_frame(display_draw_fn draw, void *ctx)
{
    const int n = CANVAS_W * STRIP_ROWS;
    bool first = !s_shown_valid;
    for (int i = 0; i < STRIPS; i++) {
        canvas_t c = {.px = s_strip, .y0 = i * STRIP_ROWS, .h = STRIP_ROWS};
        draw(&c, ctx);
        uint32_t h = hash(s_strip, n);
        if (s_shown_valid && h == s_shown[i]) continue;

        for (int k = 0; k < n; k++)
            s_strip[k] = (uint16_t)((s_strip[k] >> 8) | (s_strip[k] << 8)); // big-endian on the wire
        esp_err_t err = esp_lcd_panel_draw_bitmap(s_panel, 0, c.y0, CANVAS_W, c.y0 + STRIP_ROWS, s_strip);
        if (err != ESP_OK) {
            s_shown_valid = false; // unknown now: resend everything next time
            return err;
        }
        xSemaphoreTake(s_sent, portMAX_DELAY); // the buffer is reused for the next strip
        s_shown[i] = h;
    }
    s_shown_valid = true;
    if (first) display_backlight(100);
    return ESP_OK;
}
