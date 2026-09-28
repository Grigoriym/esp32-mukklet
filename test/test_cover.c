#include <string.h>
#include "unity.h"
#include "cover.h"

static uint8_t buf[PROTO_COVER_BYTES];
static cover_t cover;

void setUp(void)
{
    memset(buf, 0, sizeof(buf));
    cover_init(&cover, buf);
}

void tearDown(void)
{
}

static proto_msg_t cover_msg(const char *id)
{
    proto_msg_t m = {.type = MSG_COVER};
    strcpy(m.cover.track_id, id);
    m.cover.w = m.cover.h = PROTO_COVER_SIZE;
    m.cover.rgb565 = true;
    m.cover.size = PROTO_COVER_BYTES;
    return m;
}

// Sends the whole cover in maxChunk frames, byte i = fill + i.
static void send_all(uint8_t fill)
{
    for (size_t off = 0; off < PROTO_COVER_BYTES; off += PROTO_MAX_CHUNK) {
        size_t n = PROTO_COVER_BYTES - off < PROTO_MAX_CHUNK ? PROTO_COVER_BYTES - off : PROTO_MAX_CHUNK;
        uint8_t *dst = cover_chunk_dst(&cover, n);
        TEST_ASSERT_EQUAL_PTR(buf + off, dst);
        for (size_t i = 0; i < n; i++) dst[i] = (uint8_t)(fill + off + i);
        TEST_ASSERT_EQUAL(off + n == PROTO_COVER_BYTES, cover_chunk_done(&cover, n));
    }
}

static void test_assembles_chunks_for_its_track(void)
{
    TEST_ASSERT_NULL(cover_for(&cover, ""));
    proto_msg_t m = cover_msg("a3f9c2");
    cover_begin(&cover, &m);
    TEST_ASSERT_NULL(cover_for(&cover, "a3f9c2")); // not before the last byte
    send_all(7);
    TEST_ASSERT_EQUAL_PTR(buf, cover_for(&cover, "a3f9c2"));
    TEST_ASSERT_NULL(cover_for(&cover, "other"));
    TEST_ASSERT_EQUAL_HEX8(7, buf[0]);
    TEST_ASSERT_EQUAL_HEX8((uint8_t)(7 + PROTO_COVER_BYTES - 1), buf[PROTO_COVER_BYTES - 1]);
    TEST_ASSERT_NULL(cover_chunk_dst(&cover, 1)); // nothing more due
}

static void test_new_cover_replaces_old(void)
{
    proto_msg_t a = cover_msg("a"), b = cover_msg("b");
    cover_begin(&cover, &a);
    send_all(0);
    cover_begin(&cover, &b);
    TEST_ASSERT_NULL(cover_for(&cover, "a")); // being overwritten
    TEST_ASSERT_NULL(cover_for(&cover, "b"));
    send_all(1);
    TEST_ASSERT_EQUAL_PTR(buf, cover_for(&cover, "b"));
}

static void test_none_or_other_format_is_no_art(void)
{
    proto_msg_t m = cover_msg("a");
    cover_begin(&cover, &m);
    send_all(0);
    proto_msg_t none = {.type = MSG_COVER};
    strcpy(none.cover.track_id, "a");
    none.cover.none = true;
    cover_begin(&cover, &none);
    TEST_ASSERT_NULL(cover_for(&cover, "a"));
    TEST_ASSERT_NULL(cover_chunk_dst(&cover, 16));

    m.cover.rgb565 = false; // mono1 64x64, say
    cover_begin(&cover, &m);
    TEST_ASSERT_NULL(cover_chunk_dst(&cover, 16));
    m = cover_msg("a");
    m.cover.w = 240;
    cover_begin(&cover, &m);
    TEST_ASSERT_NULL(cover_chunk_dst(&cover, 16));
    m = cover_msg("a");
    m.cover.size = 100;
    cover_begin(&cover, &m);
    TEST_ASSERT_NULL(cover_chunk_dst(&cover, 16));
}

static void test_oversized_chunks_rejected(void)
{
    proto_msg_t m = cover_msg("a");
    cover_begin(&cover, &m);
    TEST_ASSERT_NULL(cover_chunk_dst(&cover, PROTO_MAX_CHUNK + 1));
    TEST_ASSERT_NULL(cover_chunk_dst(&cover, 0));
    TEST_ASSERT_NOT_NULL(cover_chunk_dst(&cover, PROTO_MAX_CHUNK));
    // Near the end, a frame reaching past size doesn't fit.
    cover.got = PROTO_COVER_BYTES - 10;
    TEST_ASSERT_NULL(cover_chunk_dst(&cover, 11));
    TEST_ASSERT_NOT_NULL(cover_chunk_dst(&cover, 10));
}

static void test_cancel_drops_partial_keeps_finished(void)
{
    proto_msg_t m = cover_msg("a");
    cover_begin(&cover, &m);
    cover_chunk_dst(&cover, 100);
    cover_chunk_done(&cover, 100);
    cover_cancel(&cover);
    TEST_ASSERT_NULL(cover_chunk_dst(&cover, 100)); // the rest isn't taken
    TEST_ASSERT_NULL(cover_for(&cover, "a"));

    cover_begin(&cover, &m);
    send_all(0);
    cover_cancel(&cover); // e.g. the next state message
    TEST_ASSERT_EQUAL_PTR(buf, cover_for(&cover, "a"));
}

static void test_reset_forgets(void)
{
    proto_msg_t m = cover_msg("a");
    cover_begin(&cover, &m);
    send_all(0);
    cover_reset(&cover);
    TEST_ASSERT_NULL(cover_for(&cover, "a"));
    TEST_ASSERT_EQUAL_PTR(buf, cover.px);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_assembles_chunks_for_its_track);
    RUN_TEST(test_new_cover_replaces_old);
    RUN_TEST(test_none_or_other_format_is_no_art);
    RUN_TEST(test_oversized_chunks_rejected);
    RUN_TEST(test_cancel_drops_partial_keeps_finished);
    RUN_TEST(test_reset_forgets);
    return UNITY_END();
}
