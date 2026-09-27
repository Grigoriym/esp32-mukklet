#pragma once

// Draws the whole screen for a moment in time. Pure and stateless (the
// scrolling of long lines is a function of now_ms too), so the unit tests in
// test/ can render any screen as ASCII art.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "fb.h"
#include "player.h"

typedef struct {
    bool wifi;      // station connected
    const char *ip; // shown while waiting for Mukk; may be NULL
    const player_t *player;
} ui_input_t;

void ui_render(fb_t *fb, const ui_input_t *in, int64_t now_ms);

// "m:ss", or "h:mm:ss" from an hour on.
void ui_format_time(char *buf, size_t len, int32_t ms);

// How far (px) a line text_w wide is scrolled left inside a box_w box, t_ms
// after it appeared: holds, scrolls to its end, holds, jumps back, repeats.
int ui_scroll_offset(int text_w, int box_w, int64_t t_ms);
