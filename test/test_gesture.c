#include "unity.h"
#include "gesture.h"

#define POLL_MS 10

static gesture_state_t g;
static int64_t now;

void setUp(void)
{
    gesture_init(&g);
    now = 0;
}

void tearDown(void)
{
}

// Holds the level for ms, polling like the button task. Returns the one
// gesture reported in that time (fails on more than one).
static gesture_t hold(bool pressed, int ms)
{
    gesture_t seen = GESTURE_NONE;
    for (int t = 0; t < ms; t += POLL_MS) {
        gesture_t r = gesture_update(&g, pressed, now);
        now += POLL_MS;
        if (r == GESTURE_NONE) continue;
        TEST_ASSERT_EQUAL_MESSAGE(GESTURE_NONE, seen, "two gestures");
        seen = r;
    }
    return seen;
}

static void test_single_reported_after_double_window(void)
{
    TEST_ASSERT_EQUAL(GESTURE_NONE, hold(true, 100));
    TEST_ASSERT_EQUAL(GESTURE_NONE, hold(false, GESTURE_DOUBLE_MS - POLL_MS));
    TEST_ASSERT_EQUAL(GESTURE_SINGLE, hold(false, 2 * POLL_MS));
    TEST_ASSERT_EQUAL(GESTURE_NONE, hold(false, 1000));
}

static void test_double(void)
{
    hold(true, 80);
    hold(false, 150);
    TEST_ASSERT_EQUAL(GESTURE_NONE, hold(true, 80)); // reported on release
    TEST_ASSERT_EQUAL(GESTURE_DOUBLE, hold(false, 1000));
}

static void test_long_fires_while_held_release_swallowed(void)
{
    TEST_ASSERT_EQUAL(GESTURE_LONG, hold(true, GESTURE_LONG_MS + POLL_MS));
    TEST_ASSERT_EQUAL(GESTURE_NONE, hold(true, 2000)); // only once per hold
    TEST_ASSERT_EQUAL(GESTURE_NONE, hold(false, 1000));
}

static void test_second_press_held_long_is_a_double(void)
{
    hold(true, 80);
    hold(false, 100);
    TEST_ASSERT_EQUAL(GESTURE_NONE, hold(true, 1000));
    TEST_ASSERT_EQUAL(GESTURE_DOUBLE, hold(false, 500));
}

static void test_two_slow_presses_are_two_singles(void)
{
    hold(true, 80);
    TEST_ASSERT_EQUAL(GESTURE_SINGLE, hold(false, 500));
    hold(true, 80);
    TEST_ASSERT_EQUAL(GESTURE_SINGLE, hold(false, 500));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_single_reported_after_double_window);
    RUN_TEST(test_double);
    RUN_TEST(test_long_fires_while_held_release_swallowed);
    RUN_TEST(test_second_press_held_long_is_a_double);
    RUN_TEST(test_two_slow_presses_are_two_singles);
    return UNITY_END();
}
