#pragma once

// 128x64 monochrome framebuffer in the SSD1306's own memory layout, so
// display.c can send it as is. No ESP-IDF dependencies (host-testable).

#include <stdbool.h>
#include <stdint.h>

#define FB_W     128
#define FB_H     64
#define FB_PAGES (FB_H / 8)

typedef struct {
    // 8 pages of 8 rows; each byte is one column of a page, bit0 = top row.
    uint8_t page[FB_PAGES][FB_W];
} fb_t;

void fb_clear(fb_t *fb);

bool fb_get(const fb_t *fb, int x, int y);

// Out-of-bounds pixels are ignored by all drawing below.
void fb_set(fb_t *fb, int x, int y, bool on);

void fb_fill(fb_t *fb, int x, int y, int w, int h, bool on);

// Draws UTF-8 text with its top-left corner at (x, y), only the columns in
// [clip_x0, clip_x1) (for scrolling text within a box). Returns the width
// of the whole text in px.
int fb_text_clipped(fb_t *fb, int x, int y, const char *text, int clip_x0, int clip_x1);

// fb_text_clipped() over the full width.
int fb_text(fb_t *fb, int x, int y, const char *text);

// Right edge of the text at x_right (exclusive).
void fb_text_right(fb_t *fb, int x_right, int y, const char *text);

// Centered horizontally.
void fb_text_center(fb_t *fb, int y, const char *text);
