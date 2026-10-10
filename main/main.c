#include "display.h"
#include "encoder.h"
#include "link.h"
#include "ui.h"
#include "wifi.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "main";

#define WIFI_TIMEOUT_MS 10000 // then carry on, it keeps trying in the background
#define FRAME_MS        50    // redraw at up to 20 fps: smooth scrolling and progress
#define VOLUME_STEP     5     // percentage points per knob detent
#define EDGE_TEST       0     // 1: a fixed test screen to check which pixels the case's window shows

static player_t s_player;
static bool s_display_ok;
static ui_screen_t s_screen = UI_CONNECTING;
static int64_t s_screen_since; // when s_screen came up
static bool s_dark;            // screen turned off with a hold

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

struct frame {
    ui_input_t in;
    int64_t now_ms;
};

#if EDGE_TEST
// Frames 2 px wide from each edge inwards, one colour each, grey inside:
// the outermost colour seen on a side tells how many pixels the window
// hides there, the grey shows where the lit area ends against the glass.
static void draw_edge_test(canvas_t *c)
{
    static const uint16_t ring[] = {RGB(255, 0, 0),   RGB(0, 255, 0),   RGB(0, 80, 255),   RGB(255, 255, 0),
                                    RGB(255, 0, 255), RGB(0, 255, 255), RGB(255, 255, 255)};
    const int n = sizeof(ring) / sizeof(ring[0]);
    canvas_fill(c, 0, 0, CANVAS_W, CANVAS_H, RGB(110, 110, 110));
    for (int i = 0; i < n; i++) {
        int d = 2 * i;
        canvas_fill(c, d, d, CANVAS_W - 2 * d, 2, ring[i]);
        canvas_fill(c, d, CANVAS_H - d - 2, CANVAS_W - 2 * d, 2, ring[i]);
        canvas_fill(c, d, d, 2, CANVAS_H - 2 * d, ring[i]);
        canvas_fill(c, CANVAS_W - d - 2, d, 2, CANVAS_H - 2 * d, ring[i]);
    }
    canvas_text_center(c, &FONT_TITLE, 90, "Edge test", RGB(255, 255, 255));
    canvas_text_center(c, &FONT_TEXT, 130, "outside in, 2 px each:", RGB(230, 230, 230));
    canvas_text_center(c, &FONT_TEXT, 154, "red green blue", RGB(230, 230, 230));
    canvas_text_center(c, &FONT_TEXT, 178, "yellow magenta cyan", RGB(230, 230, 230));
    canvas_text_center(c, &FONT_TEXT, 202, "white", RGB(230, 230, 230));
}
#endif

static void draw_strip(canvas_t *c, void *ctx)
{
    const struct frame *f = ctx;
#if EDGE_TEST
    (void)f;
    draw_edge_test(c);
#else
    ui_render(c, &f->in, f->now_ms);
#endif
}

static void render(void)
{
    link_snapshot(&s_player);
    char ip[16] = "";
    struct frame f = {
        .in = {.wifi = wifi_ip(ip, sizeof(ip)), .ip = ip, .player = &s_player},
        .now_ms = now_ms(), // one moment for all strips of the frame
    };
    ui_screen_t screen = ui_screen(&f.in, f.now_ms);
    if (screen != s_screen) {
        s_screen = screen;
        s_screen_since = f.now_ms;
    }
    f.in.screen_since_ms = s_screen_since;
    f.in.cover = link_cover_acquire(s_player.has_track ? s_player.track.id : "");
    if (s_display_ok && !s_dark) display_frame(draw_strip, &f);
    link_cover_release();
}

// Backlight off, and no drawing until woken.
static void set_dark(bool dark)
{
    s_dark = dark;
    if (s_display_ok) display_backlight(dark ? 0 : 100);
}

// Knob mapping: turn = volume, press = play/pause, double = next, long =
// prev, hold = screen off. While off, any input only turns it back on.
// Waits up to wait for the first event, then takes whatever else is
// queued; a fast spin becomes one volume command.
static void handle_input(TickType_t wait)
{
    int volume = 0;
    input_event_t ev;
    while (encoder_wait_event(&ev, wait)) {
        wait = 0;
        if (s_dark) {
            s_dark = false;
            render(); // bring the panel up to date before it lights up
            set_dark(false);
            // The rest of this burst (a spin) only woke it too.
            while (encoder_wait_event(&ev, 0)) continue;
            return;
        }
        switch (ev.type) {
            case INPUT_CW: volume += VOLUME_STEP; break;
            case INPUT_CCW: volume -= VOLUME_STEP; break;
            case INPUT_PRESS: link_send_cmd(CMD_PLAY_PAUSE, 0); break;
            case INPUT_DOUBLE: link_send_cmd(CMD_NEXT, 0); break;
            case INPUT_LONG: link_send_cmd(CMD_PREV, 0); break;
            case INPUT_HOLD: set_dark(true); break;
        }
    }
    if (volume != 0) link_send_cmd(CMD_VOLUME, volume);
}

void app_main(void)
{
    esp_err_t err = display_init();
    s_display_ok = err == ESP_OK;
    if (!s_display_ok) ESP_LOGE(TAG, "display: %s, running without it", esp_err_to_name(err));
    render(); // "Connecting to WiFi" while that blocks

    err = encoder_init();
    if (err != ESP_OK) ESP_LOGE(TAG, "encoder: %s", esp_err_to_name(err));

    wifi_connect(WIFI_TIMEOUT_MS);
    ESP_ERROR_CHECK(link_start());

    for (;;) {
        handle_input(pdMS_TO_TICKS(FRAME_MS));
        render();
    }
}
