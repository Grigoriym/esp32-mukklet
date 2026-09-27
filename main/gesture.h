#pragma once

// Turns the debounced knob button level into single / double / long presses.
// Pure, times passed in, so it also compiles on the PC for the unit tests.
//
// A single press is only reported once GESTURE_DOUBLE_MS has passed without
// a second one, so it lags the release by that much. A long press fires
// while the button is still held, and its release is swallowed.

#include <stdbool.h>
#include <stdint.h>

#define GESTURE_LONG_MS   600 // held this long = long press
#define GESTURE_DOUBLE_MS 300 // second press within this after a release = double

typedef enum {
    GESTURE_NONE,
    GESTURE_SINGLE,
    GESTURE_DOUBLE,
    GESTURE_LONG,
} gesture_t;

typedef struct {
    bool down;
    bool long_fired; // this hold already reported GESTURE_LONG
    int clicks;      // releases waiting to be reported
    int64_t down_ms;
    int64_t up_ms;
} gesture_state_t;

void gesture_init(gesture_state_t *g);

// Feeds the current level (true = pressed), called on every poll.
gesture_t gesture_update(gesture_state_t *g, bool pressed, int64_t now_ms);
