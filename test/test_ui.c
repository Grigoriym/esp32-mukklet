#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "ui.h"
#include "font.h"

// SHOW_ART=1 tools/test.sh prints each rendered screen as ASCII art.

static fb_t fb;
static player_t p;
static ui_input_t in;

void setUp(void)
{
    player_init(&p);
    in = (ui_input_t){.wifi = true, .ip = "192.168.1.50", .player = &p};
}

void tearDown(void)
{
}

static void show(const char *name)
{
    if (!getenv("SHOW_ART")) return;
    printf("--- %s\n", name);
    for (int y = 0; y < FB_H; y++) {
        putchar('|');
        for (int x = 0; x < FB_W; x++) putchar(fb_get(&fb, x, y) ? '#' : ' ');
        puts("|");
    }
}

// True if text drawn alone at (x, y) matches the screen over its cells.
static bool has_text_at(int x, int y, const char *text)
{
    fb_t want;
    fb_clear(&want);
    int w = fb_text(&want, x, y, text);
    for (int yy = y; yy < y + FONT_H; yy++) {
        for (int xx = x; xx < x + w && xx < FB_W; xx++) {
            if (fb_get(&want, xx, yy) != fb_get(&fb, xx, yy)) return false;
        }
    }
    return true;
}

static bool has_centered(int y, const char *text)
{
    return has_text_at((FB_W - font_text_width(text)) / 2, y, text);
}

static void playing(void)
{
    player_session_start(&p, 0);
    proto_msg_t m = {.type = MSG_TRACK};
    m.track.present = true;
    strcpy(m.track.track.id, "a3f9c2");
    strcpy(m.track.track.title, "Paranoid Android");
    strcpy(m.track.track.artist, "Radiohead");
    strcpy(m.track.track.album, "OK Computer");
    m.track.track.year = 1997;
    m.track.track.duration_ms = 386000;
    m.track.has_next = true;
    strcpy(m.track.next.title, "Subterranean Homesick Alien");
    player_apply(&p, &m, 0);
    proto_msg_t s = {.type = MSG_STATE};
    s.state.status = PLAY_PLAYING;
    s.state.position_ms = 125000;
    s.state.volume = 80;
    player_apply(&p, &s, 0);
}

static void test_format_time(void)
{
    char buf[16];
    ui_format_time(buf, sizeof(buf), 0);
    TEST_ASSERT_EQUAL_STRING("0:00", buf);
    ui_format_time(buf, sizeof(buf), 125999);
    TEST_ASSERT_EQUAL_STRING("2:05", buf);
    ui_format_time(buf, sizeof(buf), 3600000 + 61000);
    TEST_ASSERT_EQUAL_STRING("1:01:01", buf);
}

static void test_scroll_offset(void)
{
    TEST_ASSERT_EQUAL(0, ui_scroll_offset(100, 128, 5000)); // fits: never moves
    // 60 px too wide at 30 px/s: hold 2 s, scroll 2 s, hold 2 s, repeat.
    TEST_ASSERT_EQUAL(0, ui_scroll_offset(188, 128, 1999));
    TEST_ASSERT_EQUAL(30, ui_scroll_offset(188, 128, 3000));
    TEST_ASSERT_EQUAL(60, ui_scroll_offset(188, 128, 4000));
    TEST_ASSERT_EQUAL(60, ui_scroll_offset(188, 128, 5999));
    TEST_ASSERT_EQUAL(0, ui_scroll_offset(188, 128, 6000));
}

static void test_no_wifi(void)
{
    in.wifi = false;
    ui_render(&fb, &in, 0);
    show("no wifi");
    TEST_ASSERT_TRUE(has_centered(8, "Mukklet"));
    TEST_ASSERT_TRUE(has_centered(26, "Connecting to WiFi"));
}

static void test_waiting_shows_address(void)
{
    ui_render(&fb, &in, 0);
    show("waiting");
    TEST_ASSERT_TRUE(has_centered(26, "Waiting for Mukk"));
    TEST_ASSERT_TRUE(has_centered(38, "mukklet.local"));
    TEST_ASSERT_TRUE(has_centered(50, "192.168.1.50"));
}

static void test_offline_after_silence(void)
{
    playing();
    ui_render(&fb, &in, PLAYER_TIMEOUT_MS);
    show("offline");
    TEST_ASSERT_TRUE(has_centered(26, "Mukk offline"));
}

static void test_now_playing(void)
{
    playing();
    ui_render(&fb, &in, 1000);
    show("now playing");
    TEST_ASSERT_TRUE(has_text_at(0, 0, "Paranoid Android"));
    TEST_ASSERT_TRUE(has_text_at(0, 11, "Radiohead"));
    TEST_ASSERT_TRUE(has_text_at(0, 22, "OK Computer (1997)"));
    TEST_ASSERT_TRUE(has_text_at(0, 33, "Next: Subterranean Homes")); // cut at the edge
    TEST_ASSERT_TRUE(has_text_at(FONT_W + 3, 54, "2:06"));            // 1 s after 2:05
    TEST_ASSERT_TRUE(has_centered(54, "80%"));
    TEST_ASSERT_TRUE(has_text_at(FB_W - 4 * FONT_W, 54, "6:26"));
    // Progress bar: filled to 126/386 of the width, then just the line.
    TEST_ASSERT_TRUE(fb_get(&fb, 40, 46));
    TEST_ASSERT_FALSE(fb_get(&fb, 42, 46));
    TEST_ASSERT_TRUE(fb_get(&fb, 100, 47));
}

static void test_long_line_scrolls(void)
{
    playing();
    // "Next: Subterranean Homesick Alien" is 33 chars = 198 px; at 3 s it
    // has moved 30 px.
    ui_render(&fb, &in, 3000);
    show("scrolled");
    TEST_ASSERT_TRUE(has_text_at(0, 0, "Paranoid Android")); // fits: stays
    fb_t want;
    fb_clear(&want);
    fb_text(&want, -30, 33, "Next: Subterranean Homesick Alien");
    for (int x = 0; x < FB_W; x++) {
        for (int y = 33; y < 43; y++) TEST_ASSERT_EQUAL(fb_get(&want, x, y), fb_get(&fb, x, y));
    }
}

static void test_nothing_playing(void)
{
    player_session_start(&p, 0);
    proto_msg_t s = {.type = MSG_STATE};
    s.state.status = PLAY_IDLE;
    s.state.volume = 55;
    player_apply(&p, &s, 0);
    ui_render(&fb, &in, 0);
    show("nothing playing");
    TEST_ASSERT_TRUE(has_centered(16, "Nothing playing"));
    TEST_ASSERT_TRUE(has_centered(54, "55%"));
}

static void test_cyrillic_title(void)
{
    playing();
    strcpy(p.track.title, "Группа крови");
    ui_render(&fb, &in, 0);
    show("cyrillic");
    TEST_ASSERT_TRUE(has_text_at(0, 0, "Группа крови"));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_format_time);
    RUN_TEST(test_scroll_offset);
    RUN_TEST(test_no_wifi);
    RUN_TEST(test_waiting_shows_address);
    RUN_TEST(test_offline_after_silence);
    RUN_TEST(test_now_playing);
    RUN_TEST(test_long_line_scrolls);
    RUN_TEST(test_nothing_playing);
    RUN_TEST(test_cyrillic_title);
    return UNITY_END();
}
