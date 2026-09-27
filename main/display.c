#include <string.h>
#include "display.h"

#define OLED_WIDTH FB_W
#define I2C_HZ     400000 // fast mode: a full frame in ~30 ms

static i2c_master_dev_handle_t s_dev;
static fb_t s_shown; // what the panel has, see display_show()
static bool s_shown_valid;

// Orientation: A0/C0 below = rotated 180 from the panel default, as on the
// desk display; A1/C8 for the other way up.
static const uint8_t ssd1306_init_cmds[] = {
    0xAE,       // display off
    0xD5, 0x80, // clock divide
    0xA8, 0x3F, // multiplex ratio = 63 (64px tall)
    0xD3, 0x00, // display offset = 0
    0x40,       // start line = 0
    0x8D, 0x14, // charge pump on
    0x20, 0x02, // page addressing mode
    0xA0,       // segment remap
    0xC0,       // COM scan direction
    0xDA, 0x12, // COM pins config for 128x64
    0x81, 0x40, // contrast (0x00-0xFF; 0x40 looks the same as 0xCF indoors)
    0xD9, 0xF1, // precharge
    0xDB, 0x40, // VCOMH deselect level
    0xA4,       // resume to RAM content display
    0xA6,       // normal (not inverted)
    0xAF,       // display on
};

static esp_err_t ssd1306_cmd(uint8_t cmd)
{
    uint8_t buf[2] = {0x00, cmd}; // control byte 0x00 = command follows
    return i2c_master_transmit(s_dev, buf, sizeof(buf), 1000);
}

static esp_err_t ssd1306_write_page(int page, const uint8_t *row, size_t len)
{
    uint8_t buf[1 + OLED_WIDTH];
    buf[0] = 0x40; // control byte 0x40 = data follows
    memcpy(&buf[1], row, len);

    esp_err_t err;
    if ((err = ssd1306_cmd(0xB0 | page)) != ESP_OK) return err; // set page
    if ((err = ssd1306_cmd(0x00)) != ESP_OK) return err;        // lower column = 0
    if ((err = ssd1306_cmd(0x10)) != ESP_OK) return err;        // upper column = 0
    return i2c_master_transmit(s_dev, buf, len + 1, 1000);
}

esp_err_t display_init(i2c_master_bus_handle_t bus)
{
    uint16_t addr = 0;
    if (i2c_master_probe(bus, 0x3C, 50) == ESP_OK) {
        addr = 0x3C;
    } else if (i2c_master_probe(bus, 0x3D, 50) == ESP_OK) {
        addr = 0x3D;
    } else {
        return ESP_ERR_NOT_FOUND;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_HZ,
    };
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &s_dev);
    if (err != ESP_OK) return err;

    for (size_t i = 0; i < sizeof(ssd1306_init_cmds); i++) {
        if ((err = ssd1306_cmd(ssd1306_init_cmds[i])) != ESP_OK) return err;
    }
    return ESP_OK;
}

esp_err_t display_show(const fb_t *fb)
{
    for (int page = 0; page < FB_PAGES; page++) {
        if (s_shown_valid && memcmp(s_shown.page[page], fb->page[page], OLED_WIDTH) == 0) continue;
        esp_err_t err = ssd1306_write_page(page, fb->page[page], OLED_WIDTH);
        if (err != ESP_OK) {
            s_shown_valid = false; // unknown now: resend everything next time
            return err;
        }
        memcpy(s_shown.page[page], fb->page[page], OLED_WIDTH);
    }
    s_shown_valid = true;
    return ESP_OK;
}
