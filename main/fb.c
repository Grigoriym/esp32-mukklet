#include <string.h>
#include "fb.h"
#include "font.h"

void fb_clear(fb_t *fb)
{
    memset(fb, 0, sizeof(*fb));
}

bool fb_get(const fb_t *fb, int x, int y)
{
    if (x < 0 || x >= FB_W || y < 0 || y >= FB_H) return false;
    return fb->page[y / 8][x] & (1 << (y % 8));
}

void fb_set(fb_t *fb, int x, int y, bool on)
{
    if (x < 0 || x >= FB_W || y < 0 || y >= FB_H) return;
    uint8_t bit = 1 << (y % 8);
    if (on) {
        fb->page[y / 8][x] |= bit;
    } else {
        fb->page[y / 8][x] &= ~bit;
    }
}

void fb_fill(fb_t *fb, int x, int y, int w, int h, bool on)
{
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) fb_set(fb, xx, yy, on);
    }
}

int fb_text_clipped(fb_t *fb, int x, int y, const char *text, int clip_x0, int clip_x1)
{
    int start = x;
    uint32_t cp;
    while ((cp = utf8_next(&text)) != 0) {
        if (x + FONT_W > clip_x0 && x < clip_x1) {
            const uint8_t *rows = font_glyph(cp);
            for (int r = 0; r < FONT_H; r++) {
                for (int c = 0; c < FONT_W; c++) {
                    int px = x + c;
                    if (px >= clip_x0 && px < clip_x1 && (rows[r] & (0x80 >> c))) fb_set(fb, px, y + r, true);
                }
            }
        }
        x += FONT_W;
    }
    return x - start;
}

int fb_text(fb_t *fb, int x, int y, const char *text)
{
    return fb_text_clipped(fb, x, y, text, 0, FB_W);
}

void fb_text_right(fb_t *fb, int x_right, int y, const char *text)
{
    fb_text(fb, x_right - font_text_width(text), y, text);
}

void fb_text_center(fb_t *fb, int y, const char *text)
{
    fb_text(fb, (FB_W - font_text_width(text)) / 2, y, text);
}
