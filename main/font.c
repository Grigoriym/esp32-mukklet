#include <stddef.h>
#include "font.h"

uint32_t utf8_next(const char **p)
{
    const unsigned char *s = (const unsigned char *)*p;
    if (!s || !*s) return 0;

    int extra;
    uint32_t cp;
    if (s[0] < 0x80) {
        extra = 0;
        cp = s[0];
    } else if ((s[0] & 0xE0) == 0xC0) {
        extra = 1;
        cp = s[0] & 0x1F;
    } else if ((s[0] & 0xF0) == 0xE0) {
        extra = 2;
        cp = s[0] & 0x0F;
    } else if ((s[0] & 0xF8) == 0xF0) {
        extra = 3;
        cp = s[0] & 0x07;
    } else {
        *p += 1; // stray continuation byte or invalid lead
        return FONT_MISSING;
    }
    for (int i = 1; i <= extra; i++) {
        if ((s[i] & 0xC0) != 0x80) { // truncated sequence (also stops at the NUL)
            *p += i;
            return FONT_MISSING;
        }
        cp = (cp << 6) | (s[i] & 0x3F);
    }
    *p += 1 + extra;
    return cp;
}

static const font_glyph_t *find(uint32_t cp)
{
    int lo = 0;
    int hi = FONT_GLYPH_COUNT - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (FONT_GLYPHS[mid].cp == cp) return &FONT_GLYPHS[mid];
        if (FONT_GLYPHS[mid].cp < cp) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return NULL;
}

// Punctuation that tags often use and the font lacks (it does have ’ “ ” „).
static uint32_t look_alike(uint32_t cp)
{
    switch (cp) {
        case 0x2018: // ‘ ‚ ′
        case 0x201A:
        case 0x2032: return '\'';
        case 0x2033: return '"'; // ″
        case 0x2010:             // hyphens and dashes, minus sign
        case 0x2011:
        case 0x2012:
        case 0x2013:
        case 0x2014:
        case 0x2015:
        case 0x2212: return '-';
        case 0x2022: return 0x00B7; // bullet -> middle dot
        default: return FONT_MISSING;
    }
}

const uint8_t *font_glyph(uint32_t cp)
{
    const font_glyph_t *g = find(cp);
    if (!g) g = find(look_alike(cp));
    return g->rows; // FONT_MISSING is always in the table
}

int font_text_width(const char *text)
{
    int n = 0;
    while (utf8_next(&text)) n++;
    return n * FONT_W;
}
