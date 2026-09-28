#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "ui.h"
#include "font.h"

// SHOW_ART=1 tools/test.sh prints each rendered screen as ASCII art ('#'
// bright, '+' mid, '.' dark, ' ' black).

// Layout rows from ui.c.
#define Y_TITLE  174
#define Y_ARTIST 200
#define Y_ALBUM  220
#define Y_BAR    246
#define Y_BOTTOM 255
#define MARGIN   12
#define C_ACCENT RGB(255, 150, 40)
#define C_TRACK  RGB(60, 60, 60)

static uint16_t screen_px[CANVAS_W * CANVAS_H];
static canvas_t screen = {.px = screen_px, .y0 = 0, .h = CANVAS_H};
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

static int luma(uint16_t c)
{
    return ((c >> 11) * 255 / 31 * 3 + ((c >> 5) & 0x3F) * 255 / 63 * 6 + (c & 0x1F) * 255 / 31) / 10;
}

static void show(const char *name)
{
    if (!getenv("SHOW_ART")) return;
    printf("--- %s\n", name);
    for (int y = 0; y < CANVAS_H; y++) {
        putchar('|');
        for (int x = 0; x < CANVAS_W; x++) {
            int l = luma(canvas_get(&screen, x, y));
            putchar(l > 170 ? '#' : l > 90 ? '+' : l > 0 ? '.' : ' ');
        }
        puts("|");
    }
}

static void render(int64_t now_ms)
{
    ui_render(&screen, &in, now_ms);
}

// True if text drawn alone (on black) at (x, y) matches the screen over
// its line, so nothing else overlaps it either.
static bool has_text_at(const font_t *font, int x, int y, const char *text)
{
    static uint16_t want_px[CANVAS_W * CANVAS_H];
    canvas_t want = {.px = want_px, .y0 = 0, .h = CANVAS_H};
    canvas_fill(&want, 0, 0, CANVAS_W, CANVAS_H, 0);
    // Colour doesn't matter: compare where the text has ink.
    int w = canvas_text(&want, font, x, y, text, RGB(255, 255, 255));
    int ink = 0;
    for (int yy = y; yy < y + font->line_h; yy++) {
        for (int xx = x < 0 ? 0 : x; xx < x + w && xx < CANVAS_W; xx++) {
            bool a = canvas_get(&want, xx, yy) != 0;
            bool b = canvas_get(&screen, xx, yy) != 0;
            if (a != b) return false;
            ink += a;
        }
    }
    return ink > 0;
}

static bool has_centered(const font_t *font, int y, const char *text)
{
    return has_text_at(font, (CANVAS_W - font_text_width(font, text)) / 2, y, text);
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
    render(0);
    show("no wifi");
    TEST_ASSERT_TRUE(has_centered(&FONT_TITLE, 80, "Mukklet"));
    TEST_ASSERT_TRUE(has_centered(&FONT_TEXT, 124, "Connecting to WiFi"));
}

static void test_waiting_shows_address(void)
{
    render(0);
    show("waiting");
    TEST_ASSERT_TRUE(has_centered(&FONT_TEXT, 124, "Waiting for Mukk"));
    TEST_ASSERT_TRUE(has_centered(&FONT_TEXT, 148, "mukklet.local"));
    TEST_ASSERT_TRUE(has_centered(&FONT_TEXT, 170, "192.168.1.50"));
}

static void test_offline_after_silence(void)
{
    playing();
    render(PLAYER_TIMEOUT_MS);
    show("offline");
    TEST_ASSERT_TRUE(has_centered(&FONT_TEXT, 124, "Mukk offline"));
}

static void test_now_playing(void)
{
    playing();
    render(1000);
    show("now playing");
    TEST_ASSERT_TRUE(has_centered(&FONT_TITLE, Y_TITLE, "Paranoid Android"));
    TEST_ASSERT_TRUE(has_centered(&FONT_TEXT, Y_ARTIST, "Radiohead"));
    TEST_ASSERT_TRUE(has_centered(&FONT_TEXT, Y_ALBUM, "OK Computer (1997)"));
    TEST_ASSERT_TRUE(has_text_at(&FONT_TEXT, 18 + 12 + 5, Y_BOTTOM, "2:06")); // 1 s after 2:05
    TEST_ASSERT_TRUE(has_centered(&FONT_TEXT, Y_BOTTOM, "80%"));
    TEST_ASSERT_TRUE(
        has_text_at(&FONT_TEXT, CANVAS_W - 18 - font_text_width(&FONT_TEXT, "6:26"), Y_BOTTOM, "6:26"));
    // Progress bar: 126/386 of 216 px = 70 px done, the rest is track.
    TEST_ASSERT_EQUAL_HEX16(C_ACCENT, canvas_get(&screen, MARGIN + 69, Y_BAR));
    TEST_ASSERT_EQUAL_HEX16(C_TRACK, canvas_get(&screen, MARGIN + 71, Y_BAR));
    TEST_ASSERT_EQUAL_HEX16(C_TRACK, canvas_get(&screen, CANVAS_W - MARGIN - 1, Y_BAR + 3));
    TEST_ASSERT_EQUAL_HEX16(0, canvas_get(&screen, CANVAS_W - MARGIN, Y_BAR));
    // The cover's place is held.
    TEST_ASSERT_NOT_EQUAL(0, canvas_get(&screen, CANVAS_W / 2, 80));
}

static void test_cover_art_drawn(void)
{
    // A horizontal red-to-blue ramp, big-endian like it comes from Mukk.
    static uint8_t art[PROTO_COVER_BYTES];
    for (int y = 0; y < PROTO_COVER_SIZE; y++) {
        for (int x = 0; x < PROTO_COVER_SIZE; x++) {
            uint16_t c = RGB(255 - x * 255 / 159, 0, x * 255 / 159);
            art[(y * PROTO_COVER_SIZE + x) * 2] = c >> 8;
            art[(y * PROTO_COVER_SIZE + x) * 2 + 1] = c & 0xFF;
        }
    }
    playing();
    in.cover = art;
    render(1000);
    show("cover");
    const int x0 = (CANVAS_W - PROTO_COVER_SIZE) / 2, y0 = 8;
    TEST_ASSERT_EQUAL_HEX16(RGB(255, 0, 0), canvas_get(&screen, x0, y0));
    TEST_ASSERT_EQUAL_HEX16(RGB(0, 0, 255), canvas_get(&screen, x0 + 159, y0 + 159));
    TEST_ASSERT_EQUAL_HEX16(0, canvas_get(&screen, x0 - 1, y0)); // nothing around it
    TEST_ASSERT_EQUAL_HEX16(0, canvas_get(&screen, x0, y0 + 160));
    TEST_ASSERT_TRUE(has_centered(&FONT_TITLE, Y_TITLE, "Paranoid Android"));
}

static void test_long_line_scrolls(void)
{
    playing();
    strcpy(p.track.title, "Subterranean Homesick Alien (Remastered)");
    int w = font_text_width(&FONT_TITLE, p.track.title);
    TEST_ASSERT_TRUE(w > CANVAS_W - 2 * MARGIN);
    render(0);
    show("long title");
    TEST_ASSERT_TRUE(has_text_at(&FONT_TITLE, MARGIN, Y_TITLE, "Subterranean")); // starts at the margin
    render(3000); // 1 s into scrolling at 30 px/s
    show("long title scrolled");
    static uint16_t want_px[CANVAS_W * CANVAS_H];
    canvas_t want = {.px = want_px, .y0 = 0, .h = CANVAS_H};
    canvas_fill(&want, 0, 0, CANVAS_W, CANVAS_H, 0);
    canvas_text_clipped(&want, &FONT_TITLE, MARGIN - 30, Y_TITLE, p.track.title, RGB(255, 255, 255), MARGIN,
                        CANVAS_W - MARGIN);
    for (int y = Y_TITLE; y < Y_TITLE + FONT_TITLE.line_h; y++) {
        for (int x = 0; x < CANVAS_W; x++) {
            TEST_ASSERT_EQUAL(canvas_get(&want, x, y), canvas_get(&screen, x, y));
        }
    }
}

// The device draws a frame as 14 bands of 20 rows; together they must be
// exactly the full-height render.
static void test_strips_match_full_render(void)
{
    playing();
    strcpy(p.track.title, "Группа крови — Кино");
    render(4321);
    static uint16_t band_px[CANVAS_W * 20];
    for (int y0 = 0; y0 < CANVAS_H; y0 += 20) {
        canvas_t band = {.px = band_px, .y0 = y0, .h = 20};
        ui_render(&band, &in, 4321);
        TEST_ASSERT_EQUAL_MEMORY(&screen_px[y0 * CANVAS_W], band_px, sizeof(band_px));
    }
}

static void test_nothing_playing(void)
{
    player_session_start(&p, 0);
    proto_msg_t s = {.type = MSG_STATE};
    s.state.status = PLAY_IDLE;
    s.state.volume = 55;
    player_apply(&p, &s, 0);
    render(0);
    show("nothing playing");
    TEST_ASSERT_TRUE(has_centered(&FONT_TEXT, 124, "Nothing playing"));
    TEST_ASSERT_TRUE(has_centered(&FONT_TEXT, Y_BOTTOM, "55%"));
}

static void test_cyrillic_title(void)
{
    playing();
    strcpy(p.track.title, "Группа крови");
    render(0);
    show("cyrillic");
    TEST_ASSERT_TRUE(has_centered(&FONT_TITLE, Y_TITLE, "Группа крови"));
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
    RUN_TEST(test_cover_art_drawn);
    RUN_TEST(test_long_line_scrolls);
    RUN_TEST(test_strips_match_full_render);
    RUN_TEST(test_nothing_playing);
    RUN_TEST(test_cyrillic_title);
    return UNITY_END();
}
