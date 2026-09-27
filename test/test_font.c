#include <string.h>
#include "unity.h"
#include "font.h"

void setUp(void)
{
}

void tearDown(void)
{
}

static uint32_t decode_one(const char *s, int *consumed)
{
    const char *p = s;
    uint32_t cp = utf8_next(&p);
    *consumed = (int)(p - s);
    return cp;
}

static void test_utf8_lengths(void)
{
    int n;
    TEST_ASSERT_EQUAL_HEX32('A', decode_one("A", &n));
    TEST_ASSERT_EQUAL(1, n);
    TEST_ASSERT_EQUAL_HEX32(0x0416, decode_one("Ж", &n)); // Cyrillic Zhe
    TEST_ASSERT_EQUAL(2, n);
    TEST_ASSERT_EQUAL_HEX32(0x20AC, decode_one("€", &n));
    TEST_ASSERT_EQUAL(3, n);
    TEST_ASSERT_EQUAL_HEX32(0x1F3B5, decode_one("🎵", &n));
    TEST_ASSERT_EQUAL(4, n);
    TEST_ASSERT_EQUAL_HEX32(0, decode_one("", &n));
    TEST_ASSERT_EQUAL(0, n);
}

static void test_utf8_malformed_never_overruns(void)
{
    int n;
    // Stray continuation byte: skipped alone.
    TEST_ASSERT_EQUAL_HEX32(FONT_MISSING, decode_one("\x80"
                                                     "A",
                                                     &n));
    TEST_ASSERT_EQUAL(1, n);
    // Sequence cut short by the end of the string: stops at the NUL.
    TEST_ASSERT_EQUAL_HEX32(FONT_MISSING, decode_one("\xD0", &n));
    TEST_ASSERT_EQUAL(1, n);
    // Cut short by a new character: that character is kept.
    const char *p = "\xE2\x82"
                    "B";
    TEST_ASSERT_EQUAL_HEX32(FONT_MISSING, utf8_next(&p));
    TEST_ASSERT_EQUAL_HEX32('B', utf8_next(&p));
    TEST_ASSERT_EQUAL_HEX32(0, utf8_next(&p));
}

static void test_table_sorted_and_complete(void)
{
    for (int i = 1; i < FONT_GLYPH_COUNT; i++) TEST_ASSERT_TRUE(FONT_GLYPHS[i - 1].cp < FONT_GLYPHS[i].cp);
    // Every printable ASCII character, Russian/Ukrainian letters, Latin
    // letters with diacritics from tags, and the hand-drawn extras.
    const uint8_t *missing = font_glyph(FONT_MISSING);
    for (uint32_t c = 0x20; c < 0x7F; c++) TEST_ASSERT_TRUE_MESSAGE(font_glyph(c) != missing, "ASCII");
    for (uint32_t c = 0x0410; c <= 0x044F; c++)
        TEST_ASSERT_TRUE_MESSAGE(font_glyph(c) != missing, "Cyrillic");
    const char *extra = "ЁёЇїІіЄєÄÖÜßéñçŁłŠšŽžČčŐőĞğ…";
    uint32_t cp;
    while ((cp = utf8_next(&extra)) != 0) TEST_ASSERT_TRUE(font_glyph(cp) != missing);
}

static void test_look_alikes_and_missing(void)
{
    TEST_ASSERT_EQUAL_PTR(font_glyph('\''), font_glyph(0x2018)); // ‘
    TEST_ASSERT_EQUAL_PTR(font_glyph('"'), font_glyph(0x2033));  // ″
    TEST_ASSERT_NOT_EQUAL(font_glyph('\''), font_glyph(0x2019)); // ’ is in the font
    TEST_ASSERT_EQUAL_PTR(font_glyph('-'), font_glyph(0x2014));  // —
    TEST_ASSERT_EQUAL_PTR(font_glyph(FONT_MISSING), font_glyph(0x1F3B5));
    TEST_ASSERT_EQUAL_PTR(font_glyph(FONT_MISSING), font_glyph(0x65E5)); // CJK
}

static void test_glyph_shape(void)
{
    // 'H': two verticals and a crossbar, the 6th column is the gap.
    const uint8_t *h = font_glyph('H');
    TEST_ASSERT_EQUAL_HEX8(0x00, h[0]);
    TEST_ASSERT_EQUAL_HEX8(0x88, h[1]); // #...#.
    TEST_ASSERT_EQUAL_HEX8(0xF8, h[4]); // #####.
    for (int r = 0; r < FONT_H; r++) TEST_ASSERT_EQUAL_HEX8(0, h[r] & 0x07);
}

static void test_text_width_counts_characters_not_bytes(void)
{
    TEST_ASSERT_EQUAL(0, font_text_width(NULL));
    TEST_ASSERT_EQUAL(0, font_text_width(""));
    TEST_ASSERT_EQUAL(3 * FONT_W, font_text_width("abc"));
    TEST_ASSERT_EQUAL(6 * FONT_W, font_text_width("Кино 1"));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_utf8_lengths);
    RUN_TEST(test_utf8_malformed_never_overruns);
    RUN_TEST(test_table_sorted_and_complete);
    RUN_TEST(test_look_alikes_and_missing);
    RUN_TEST(test_glyph_shape);
    RUN_TEST(test_text_width_counts_characters_not_bytes);
    return UNITY_END();
}
