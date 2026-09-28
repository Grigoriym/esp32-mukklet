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

static player_t s_player;
static bool s_display_ok;

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

struct frame {
    ui_input_t in;
    int64_t now_ms;
};

static void draw_strip(canvas_t *c, void *ctx)
{
    const struct frame *f = ctx;
    ui_render(c, &f->in, f->now_ms);
}

static void render(void)
{
    link_snapshot(&s_player);
    char ip[16] = "";
    struct frame f = {
        .in = {.wifi = wifi_ip(ip, sizeof(ip)), .ip = ip, .player = &s_player},
        .now_ms = now_ms(), // one moment for all strips of the frame
    };
    f.in.cover = link_cover_acquire(s_player.has_track ? s_player.track.id : "");
    if (s_display_ok) display_frame(draw_strip, &f);
    link_cover_release();
}

// Knob mapping: turn = volume, press = play/pause, double = next, long =
// prev. Waits up to wait for the first event, then takes whatever else is
// queued; a fast spin becomes one volume command.
static void handle_input(TickType_t wait)
{
    int volume = 0;
    input_event_t ev;
    while (encoder_wait_event(&ev, wait)) {
        wait = 0;
        switch (ev.type) {
            case INPUT_CW: volume += VOLUME_STEP; break;
            case INPUT_CCW: volume -= VOLUME_STEP; break;
            case INPUT_PRESS: link_send_cmd(CMD_PLAY_PAUSE, 0); break;
            case INPUT_DOUBLE: link_send_cmd(CMD_NEXT, 0); break;
            case INPUT_LONG: link_send_cmd(CMD_PREV, 0); break;
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
