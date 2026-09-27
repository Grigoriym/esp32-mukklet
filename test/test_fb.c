#include "unity.h"
#include "fb.h"
#include "font.h"

static fb_t fb;

void setUp(void)
{
    fb_clear(&fb);
}

void tearDown(void)
{
}

static int count_lit(void)
{
    int n = 0;
    for (int y = 0; y < FB_H; y++) {
        for (int x = 0; x < FB_W; x++) n += fb_get(&fb, x, y);
    }
    return n;
}

static void test_pixel_layout_matches_ssd1306_pages(void)
{
    fb_set(&fb, 3, 0, true);
    fb_set(&fb, 3, 9, true);
    TEST_ASSERT_EQUAL_HEX8(0x01, fb.page[0][3]); // bit0 = top row of the page
    TEST_ASSERT_EQUAL_HEX8(0x02, fb.page[1][3]);
    fb_set(&fb, 3, 0, false);
    TEST_ASSERT_EQUAL_HEX8(0x00, fb.page[0][3]);
}

static void test_out_of_bounds_ignored(void)
{
    fb_set(&fb, -1, 0, true);
    fb_set(&fb, FB_W, 0, true);
    fb_set(&fb, 0, FB_H, true);
    fb_fill(&fb, -10, -10, 5, 5, true);
    TEST_ASSERT_EQUAL(0, count_lit());
    fb_fill(&fb, FB_W - 2, FB_H - 2, 10, 10, true);
    TEST_ASSERT_EQUAL(4, count_lit());
}

static void test_text_draws_the_glyph_rows(void)
{
    TEST_ASSERT_EQUAL(FONT_W, fb_text(&fb, 10, 20, "H"));
    const uint8_t *h = font_glyph('H');
    for (int r = 0; r < FONT_H; r++) {
        for (int c = 0; c < FONT_W; c++) {
            TEST_ASSERT_EQUAL((h[r] & (0x80 >> c)) != 0, fb_get(&fb, 10 + c, 20 + r));
        }
    }
}

static void test_text_clipped_to_box(void)
{
    // Starts left of the screen and is cut at x=20: nothing outside [0, 20).
    int w = fb_text_clipped(&fb, -7, 0, "HHHHHHHH", 0, 20);
    TEST_ASSERT_EQUAL(8 * FONT_W, w);
    for (int y = 0; y < FB_H; y++) {
        for (int x = 20; x < FB_W; x++) TEST_ASSERT_FALSE(fb_get(&fb, x, y));
    }
    TEST_ASSERT_TRUE(count_lit() > 0);
}

static void test_right_and_center(void)
{
    fb_text_right(&fb, FB_W, 0, "H");
    TEST_ASSERT_TRUE(fb_get(&fb, FB_W - FONT_W, 1)); // left bar of the H
    fb_clear(&fb);
    fb_text_center(&fb, 0, "HH");
    TEST_ASSERT_TRUE(fb_get(&fb, (FB_W - 2 * FONT_W) / 2, 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pixel_layout_matches_ssd1306_pages);
    RUN_TEST(test_out_of_bounds_ignored);
    RUN_TEST(test_text_draws_the_glyph_rows);
    RUN_TEST(test_text_clipped_to_box);
    RUN_TEST(test_right_and_center);
    return UNITY_END();
}
