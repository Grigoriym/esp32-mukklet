#include <stdio.h>
#include "ui.h"
#include "font.h"

#define SCROLL_HOLD_MS  2000
#define SCROLL_PX_PER_S 30

// Now-playing layout, top edges in px: the cover, three text lines, the
// progress bar, then status icon, position, volume and duration along the
// bottom. The panel's corners are rounded, hence the margins.
#define COVER_SIZE PROTO_COVER_SIZE
#define Y_COVER    8
#define Y_TITLE    174
#define Y_ARTIST   200
#define Y_ALBUM    220
#define Y_BAR      246
#define BAR_H      4
#define Y_BOTTOM   255
#define MARGIN     12 // text lines
#define MARGIN_BOT 18 // bottom row, closer to the rounded corners
#define ICON_H     12

#define C_BG     RGB(0, 0, 0)
#define C_TITLE  RGB(255, 255, 255)
#define C_ARTIST RGB(210, 210, 210)
#define C_DIM    RGB(140, 140, 140)
#define C_COVER  RGB(40, 40, 40) // the cover's place while there's no art
#define C_TRACK  RGB(60, 60, 60) // progress bar background
#define C_ACCENT RGB(255, 150, 40)

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

// A line that is centred when it fits and scrolls within the margins when
// it doesn't. Empty text draws nothing.
static void scroll_line(canvas_t *c, const font_t *font, int y, const char *text, uint16_t color,
                        int64_t t_ms)
{
    int w = font_text_width(font, text);
    int box = CANVAS_W - 2 * MARGIN;
    if (w <= box) {
        canvas_text_center(c, font, y, text, color);
        return;
    }
    canvas_text_clipped(c, font, MARGIN - ui_scroll_offset(w, box, t_ms), y, text, color, MARGIN,
                        CANVAS_W - MARGIN);
}

static void message(canvas_t *c, const char *l1, const char *l2, const char *l3)
{
    canvas_text_center(c, &FONT_TITLE, 80, "Mukklet", C_TITLE);
    canvas_fill(c, 40, 110, CANVAS_W - 80, 1, C_DIM);
    if (l1) canvas_text_center(c, &FONT_TEXT, 124, l1, C_ARTIST);
    if (l2) canvas_text_center(c, &FONT_TEXT, 148, l2, C_DIM);
    if (l3) canvas_text_center(c, &FONT_TEXT, 170, l3, C_DIM);
}

// Play triangle, pause bars or stop square, ICON_H tall, top-left at (x, y).
static void status_icon(canvas_t *c, int x, int y, play_status_t status)
{
    if (status == PLAY_PLAYING) {
        for (int i = 0; i < ICON_H; i++) {
            int half = i < ICON_H / 2 ? i : ICON_H - 1 - i;
            canvas_fill(c, x, y + i, 2 + half * 2, 1, C_DIM);
        }
    } else if (status == PLAY_PAUSED) {
        canvas_fill(c, x, y, 4, ICON_H, C_DIM);
        canvas_fill(c, x + 7, y, 4, ICON_H, C_DIM);
    } else {
        canvas_fill(c, x + 1, y + 1, ICON_H - 2, ICON_H - 2, C_DIM);
    }
}

static void bottom_row(canvas_t *c, const player_t *p, int64_t now_ms)
{
    if (!p->has_state) return;
    int text_top = Y_BOTTOM + (FONT_TEXT.ascent - ICON_H) - 1; // icon sits on the baseline
    status_icon(c, MARGIN_BOT, text_top, p->status);

    char buf[16];
    if (p->has_track) {
        ui_format_time(buf, sizeof(buf), player_position_ms(p, now_ms));
        canvas_text(c, &FONT_TEXT, MARGIN_BOT + ICON_H + 5, Y_BOTTOM, buf, C_DIM);
        if (p->track.duration_ms > 0) {
            ui_format_time(buf, sizeof(buf), p->track.duration_ms);
            canvas_text_right(c, &FONT_TEXT, CANVAS_W - MARGIN_BOT, Y_BOTTOM, buf, C_DIM);
        }
    }
    snprintf(buf, sizeof(buf), "%d%%", p->volume);
    canvas_text_center(c, &FONT_TEXT, Y_BOTTOM, buf, C_DIM);
}

static void progress_bar(canvas_t *c, const player_t *p, int64_t now_ms)
{
    int w = CANVAS_W - 2 * MARGIN;
    canvas_fill(c, MARGIN, Y_BAR, w, BAR_H, C_TRACK);
    if (!p->has_state || p->track.duration_ms <= 0) return;
    int done = (int)((int64_t)player_position_ms(p, now_ms) * w / p->track.duration_ms);
    canvas_fill(c, MARGIN, Y_BAR, done, BAR_H, C_ACCENT);
}

static void now_playing(canvas_t *c, const player_t *p, const uint8_t *cover, int64_t now_ms)
{
    if (!p->has_track) {
        canvas_text_center(c, &FONT_TEXT, 124, "Nothing playing", C_ARTIST);
        bottom_row(c, p, now_ms);
        return;
    }
    int cover_x = (CANVAS_W - COVER_SIZE) / 2;
    if (cover) {
        canvas_image_be(c, cover_x, Y_COVER, COVER_SIZE, COVER_SIZE, cover);
    } else {
        canvas_fill(c, cover_x, Y_COVER, COVER_SIZE, COVER_SIZE, C_COVER);
    }

    int64_t t = now_ms - p->track_since_ms;
    scroll_line(c, &FONT_TITLE, Y_TITLE, p->track.title, C_TITLE, t);
    scroll_line(c, &FONT_TEXT, Y_ARTIST, p->track.artist, C_ARTIST, t);

    char line[PROTO_TEXT_MAX + 16]; // + " (year)"
    if (p->track.year > 0 && p->track.album[0]) {
        snprintf(line, sizeof(line), "%s (%d)", p->track.album, p->track.year);
    } else if (p->track.year > 0) {
        snprintf(line, sizeof(line), "%d", p->track.year);
    } else {
        snprintf(line, sizeof(line), "%s", p->track.album);
    }
    scroll_line(c, &FONT_TEXT, Y_ALBUM, line, C_DIM, t);

    progress_bar(c, p, now_ms);
    bottom_row(c, p, now_ms);
}

void ui_render(canvas_t *c, const ui_input_t *in, int64_t now_ms)
{
    canvas_fill(c, 0, 0, CANVAS_W, CANVAS_H, C_BG);
    const player_t *p = in->player;
    if (!in->wifi) {
        message(c, "Connecting to WiFi", NULL, NULL);
    } else if (!p->session) {
        message(c, "Waiting for Mukk", "mukklet.local", in->ip);
    } else if (!player_online(p, now_ms)) {
        message(c, "Mukk offline", NULL, NULL);
    } else {
        now_playing(c, p, in->cover, now_ms);
    }
}
