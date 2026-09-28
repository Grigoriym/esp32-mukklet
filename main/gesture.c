#include <string.h>
#include "gesture.h"

void gesture_init(gesture_state_t *g)
{
    memset(g, 0, sizeof(*g));
}

gesture_t gesture_update(gesture_state_t *g, bool pressed, int64_t now_ms)
{
    if (pressed && !g->down) {
        g->down = true;
        g->hold_fired = false;
        g->down_ms = now_ms;
        return GESTURE_NONE;
    }
    if (pressed) {
        if (!g->hold_fired && g->clicks == 0 && now_ms - g->down_ms >= GESTURE_HOLD_MS) {
            g->hold_fired = true;
            return GESTURE_HOLD;
        }
        return GESTURE_NONE;
    }
    if (g->down) {
        g->down = false;
        if (g->hold_fired) return GESTURE_NONE;
        if (g->clicks == 0 && now_ms - g->down_ms >= GESTURE_LONG_MS) return GESTURE_LONG;
        g->up_ms = now_ms;
        if (++g->clicks == 2) {
            g->clicks = 0;
            return GESTURE_DOUBLE;
        }
        return GESTURE_NONE;
    }
    if (g->clicks == 1 && now_ms - g->up_ms >= GESTURE_DOUBLE_MS) {
        g->clicks = 0;
        return GESTURE_SINGLE;
    }
    return GESTURE_NONE;
}
