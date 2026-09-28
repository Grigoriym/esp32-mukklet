#pragma once

// What the main loop reacts to: the KY-040 knob. Plain data, no ESP-IDF
// headers.

typedef enum {
    INPUT_CW,     // one detent clockwise
    INPUT_CCW,    // one detent counter-clockwise
    INPUT_PRESS,  // single press (reported GESTURE_DOUBLE_MS after release)
    INPUT_DOUBLE, // double press
    INPUT_LONG,   // held GESTURE_LONG_MS, reported on release
    INPUT_HOLD,   // held GESTURE_HOLD_MS, reported while still held
} input_type_t;

typedef struct {
    input_type_t type;
} input_event_t;
