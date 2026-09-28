#pragma once

// Draws the screen for a moment in time into one band of it (see canvas.h;
// called once per strip). Pure and stateless (the scrolling of long lines is
// a function of now_ms too), so every strip of a frame agrees, and the unit
// tests in test/ can render any screen as ASCII art.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "canvas.h"
#include "player.h"

typedef struct {
    bool wifi;      // station connected
    const char *ip; // shown while waiting for Mukk; may be NULL
    const player_t *player;
    int64_t screen_since_ms; // when the current ui_screen() came up (the idle eyes get sleepier)
    const uint8_t *cover;    // the current track's art (PROTO_COVER_BYTES, big-endian), or NULL
} ui_input_t;

typedef enum {
    UI_CONNECTING, // to WiFi
    UI_WAITING,    // for Mukk to connect (eyes)
    UI_OFFLINE,    // Mukk went silent
    UI_NOTHING,    // connected, no track (eyes)
    UI_PLAYING,
} ui_screen_t;

// Which screen ui_render() draws.
ui_screen_t ui_screen(const ui_input_t *in, int64_t now_ms);

void ui_render(canvas_t *c, const ui_input_t *in, int64_t now_ms);

// "m:ss", or "h:mm:ss" from an hour on.
void ui_format_time(char *buf, size_t len, int32_t ms);

// How far (px) a line text_w wide is scrolled left inside a box_w box, t_ms
// after it appeared: holds, scrolls to its end, holds, jumps back, repeats.
int ui_scroll_offset(int text_w, int box_w, int64_t t_ms);
