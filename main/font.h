#pragma once

// 6x10 bitmap font covering ASCII, Latin-1/2/9/13/15 and Cyrillic, looked up
// by Unicode codepoint. No ESP-IDF dependencies, so it also compiles on the
// PC for the unit tests in test/. The glyph table (font_data.c) is generated
// by tools/gen_font.py.

#include <stdint.h>

#define FONT_W        6 // px per glyph, including the 1px gap on the right
#define FONT_H        10
#define FONT_ASCENT   8 // rows above the baseline (the rest is descent)
#define FONT_ELLIPSIS 0x2026
#define FONT_MISSING  0xFFFD // drawn for anything the font doesn't have

// Playback icons, private-use codepoints so they draw like any other glyph.
#define FONT_ICON_PLAY  0xE000
#define FONT_ICON_PAUSE 0xE001
#define FONT_ICON_STOP  0xE002

typedef struct {
    uint16_t cp;
    uint8_t rows[FONT_H]; // top to bottom, 0x80 = leftmost pixel
} font_glyph_t;

extern const font_glyph_t FONT_GLYPHS[]; // sorted by cp
extern const int FONT_GLYPH_COUNT;

// Decodes the UTF-8 sequence at *p and advances past it. Returns 0 at the
// end of the string, FONT_MISSING for malformed input (skipping one byte).
uint32_t utf8_next(const char **p);

// Rows of the glyph to draw for cp: typographic quotes and dashes fall back
// to their ASCII look-alikes, anything else unknown to FONT_MISSING.
const uint8_t *font_glyph(uint32_t cp);

// Width in px of text (FONT_W per codepoint). 0 for NULL/"".
int font_text_width(const char *text);
