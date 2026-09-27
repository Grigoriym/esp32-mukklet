#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#include "input.h"

// KY-040 rotary encoder on D25 (CLK) / D26 (DT) / D27 (SW).

// Creates the event queue, then configures the pins, the rotation interrupt
// and the button polling task. The queue works even if the pins fail.
esp_err_t encoder_init(void);

// Waits up to timeout for the next event. Returns false on timeout (or
// straight away after sleeping, if encoder_init() never ran).
bool encoder_wait_event(input_event_t *ev, TickType_t timeout);
