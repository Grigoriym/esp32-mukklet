#include <string.h>
#include "unity.h"
#include "player.h"

static player_t p;

void setUp(void)
{
    player_init(&p);
    player_session_start(&p, 1000);
}

void tearDown(void)
{
}

static void apply_track(const char *id, int32_t duration_ms, int64_t now)
{
    proto_msg_t m = {.type = MSG_TRACK};
    m.track.present = true;
    strcpy(m.track.track.id, id);
    strcpy(m.track.track.title, "Title");
    m.track.track.duration_ms = duration_ms;
    player_apply(&p, &m, now);
}

static void apply_state(play_status_t status, int32_t pos, int64_t now)
{
    proto_msg_t m = {.type = MSG_STATE};
    m.state.status = status;
    m.state.position_ms = pos;
    m.state.volume = 70;
    player_apply(&p, &m, now);
}

static void test_session_starts_empty_and_online(void)
{
    TEST_ASSERT_TRUE(p.session);
    TEST_ASSERT_FALSE(p.has_track);
    TEST_ASSERT_FALSE(p.has_state);
    TEST_ASSERT_TRUE(player_online(&p, 1000));
}

static void test_offline_after_timeout_heartbeat_resets(void)
{
    TEST_ASSERT_TRUE(player_online(&p, 1000 + PLAYER_TIMEOUT_MS - 1));
    TEST_ASSERT_FALSE(player_online(&p, 1000 + PLAYER_TIMEOUT_MS));
    proto_msg_t other = {.type = MSG_OTHER}; // any message is a heartbeat
    player_apply(&p, &other, 20000);
    TEST_ASSERT_TRUE(player_online(&p, 20000 + PLAYER_TIMEOUT_MS - 1));
}

static void test_session_end_forgets_everything(void)
{
    apply_track("a", 100000, 2000);
    apply_state(PLAY_PLAYING, 0, 2000);
    player_session_end(&p);
    TEST_ASSERT_FALSE(p.session);
    TEST_ASSERT_FALSE(p.has_track);
    TEST_ASSERT_FALSE(player_online(&p, 2000));
}

static void test_position_extrapolated_while_playing_only(void)
{
    apply_track("a", 100000, 2000);
    apply_state(PLAY_PLAYING, 5000, 2000);
    TEST_ASSERT_EQUAL(5000, player_position_ms(&p, 2000));
    TEST_ASSERT_EQUAL(8500, player_position_ms(&p, 5500));
    apply_state(PLAY_PAUSED, 8000, 6000);
    TEST_ASSERT_EQUAL(8000, player_position_ms(&p, 60000));
}

static void test_position_clamped_to_duration(void)
{
    apply_track("a", 10000, 2000);
    apply_state(PLAY_PLAYING, 9000, 2000);
    TEST_ASSERT_EQUAL(10000, player_position_ms(&p, 50000));
    apply_track("b", 0, 3000); // unknown duration: no clamp
    TEST_ASSERT_EQUAL(56000, player_position_ms(&p, 49000));
}

static void test_same_track_keeps_its_start_time(void)
{
    apply_track("a", 100000, 2000);
    TEST_ASSERT_EQUAL(2000, p.track_since_ms);
    apply_track("a", 100000, 9000); // resent after a command
    TEST_ASSERT_EQUAL(2000, p.track_since_ms);
    apply_track("b", 100000, 9500);
    TEST_ASSERT_EQUAL(9500, p.track_since_ms);
}

static void test_track_null_clears(void)
{
    apply_track("a", 100000, 2000);
    proto_msg_t none = {.type = MSG_TRACK};
    player_apply(&p, &none, 3000);
    TEST_ASSERT_FALSE(p.has_track);
    apply_track("a", 100000, 4000); // back again: counts as new
    TEST_ASSERT_EQUAL(4000, p.track_since_ms);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_session_starts_empty_and_online);
    RUN_TEST(test_offline_after_timeout_heartbeat_resets);
    RUN_TEST(test_session_end_forgets_everything);
    RUN_TEST(test_position_extrapolated_while_playing_only);
    RUN_TEST(test_position_clamped_to_duration);
    RUN_TEST(test_same_track_keeps_its_start_time);
    RUN_TEST(test_track_null_clears);
    return UNITY_END();
}
