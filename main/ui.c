#include <stdio.h>
#include "ui.h"
#include "font.h"

#define SCROLL_HOLD_MS  2000
#define SCROLL_PX_PER_S 30

// Now-playing layout, top edges in px: four text lines, the progress bar,
// then status icon, position, volume and duration along the bottom.
#define Y_TITLE  0
#define Y_ARTIST 11
#define Y_ALBUM  22
#define Y_NEXT   33
#define Y_BAR    46
#define BAR_H    3
#define Y_BOTTOM 54

void ui_format_time(char *buf, size_t len, int32_t ms)
{
    int s = ms / 1000;
    if (s >= 3600) {
        snprintf(buf, len, "%d:%02d:%02d", s / 3600, s / 60 % 60, s % 60);
    } else {
        snprintf(buf, len, "%d:%02d", s / 60, s % 60);
    }
}

int ui_scroll_offset(int text_w, int box_w, int64_t t_ms)
{
    if (text_w <= box_w || t_ms < 0) return 0;
    int travel = text_w - box_w;
    int64_t scroll_ms = (int64_t)travel * 1000 / SCROLL_PX_PER_S;
    int64_t u = t_ms % (SCROLL_HOLD_MS + scroll_ms + SCROLL_HOLD_MS);
    if (u < SCROLL_HOLD_MS) return 0;
    if (u < SCROLL_HOLD_MS + scroll_ms) return (int)((u - SCROLL_HOLD_MS) * SCROLL_PX_PER_S / 1000);
    return travel;
}

// A full-width line that scrolls when it doesn't fit. Empty text draws
// nothing.
static void scroll_line(fb_t *fb, int y, const char *text, int64_t t_ms)
{
    int w = font_text_width(text);
    fb_text(fb, -ui_scroll_offset(w, FB_W, t_ms), y, text);
}

static void message(fb_t *fb, const char *l1, const char *l2, const char *l3)
{
    fb_text_center(fb, 8, "Mukklet");
    fb_fill(fb, 0, 21, FB_W, 1, true);
    if (l1) fb_text_center(fb, 26, l1);
    if (l2) fb_text_center(fb, 38, l2);
    if (l3) fb_text_center(fb, 50, l3);
}

static void bottom_row(fb_t *fb, const player_t *p, int64_t now_ms)
{
    if (!p->has_state) return;
    char icon[4];
    uint32_t cp = p->status == PLAY_PLAYING  ? FONT_ICON_PLAY
                  : p->status == PLAY_PAUSED ? FONT_ICON_PAUSE
                                             : FONT_ICON_STOP;
    // Private-use codepoints are 3 bytes of UTF-8.
    icon[0] = (char)(0xE0 | (cp >> 12));
    icon[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    icon[2] = (char)(0x80 | (cp & 0x3F));
    icon[3] = '\0';
    fb_text(fb, 0, Y_BOTTOM, icon);

    char buf[16];
    if (p->has_track) {
        ui_format_time(buf, sizeof(buf), player_position_ms(p, now_ms));
        fb_text(fb, FONT_W + 3, Y_BOTTOM, buf);
        if (p->track.duration_ms > 0) {
            ui_format_time(buf, sizeof(buf), p->track.duration_ms);
            fb_text_right(fb, FB_W, Y_BOTTOM, buf);
        }
    }
    snprintf(buf, sizeof(buf), "%d%%", p->volume);
    fb_text_center(fb, Y_BOTTOM, buf);
}

static void progress_bar(fb_t *fb, const player_t *p, int64_t now_ms)
{
    fb_fill(fb, 0, Y_BAR + BAR_H / 2, FB_W, 1, true); // the track
    if (!p->has_state || p->track.duration_ms <= 0) return;
    int w = (int)((int64_t)player_position_ms(p, now_ms) * FB_W / p->track.duration_ms);
    fb_fill(fb, 0, Y_BAR, w, BAR_H, true);
}

static void now_playing(fb_t *fb, const player_t *p, int64_t now_ms)
{
    if (!p->has_track) {
        fb_text_center(fb, 16, "Nothing playing");
        bottom_row(fb, p, now_ms);
        return;
    }
    int64_t t = now_ms - p->track_since_ms;
    scroll_line(fb, Y_TITLE, p->track.title, t);
    scroll_line(fb, Y_ARTIST, p->track.artist, t);

    char line[PROTO_TEXT_MAX + 16]; // + " (year)" or "Next: "
    if (p->track.year > 0 && p->track.album[0]) {
        snprintf(line, sizeof(line), "%s (%d)", p->track.album, p->track.year);
    } else if (p->track.year > 0) {
        snprintf(line, sizeof(line), "%d", p->track.year);
    } else {
        snprintf(line, sizeof(line), "%s", p->track.album);
    }
    scroll_line(fb, Y_ALBUM, line, t);

    if (p->has_next) {
        snprintf(line, sizeof(line), "Next: %s", p->next.title);
        scroll_line(fb, Y_NEXT, line, t);
    }
    progress_bar(fb, p, now_ms);
    bottom_row(fb, p, now_ms);
}

void ui_render(fb_t *fb, const ui_input_t *in, int64_t now_ms)
{
    fb_clear(fb);
    const player_t *p = in->player;
    if (!in->wifi) {
        message(fb, "Connecting to WiFi", NULL, NULL);
    } else if (!p->session) {
        message(fb, "Waiting for Mukk", "mukklet.local", in->ip);
    } else if (!player_online(p, now_ms)) {
        message(fb, "Mukk offline", NULL, NULL);
    } else {
        now_playing(fb, p, now_ms);
    }
}
