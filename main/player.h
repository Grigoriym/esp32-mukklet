#pragma once

// What Mukk is playing, as far as the display knows: the latest track and
// state messages of the current connection, plus timing to extrapolate the
// position and to notice a silent Mukk. Pure, times are passed in (ms since
// boot), so it also compiles on the PC for the unit tests in test/.

#include <stdbool.h>
#include <stdint.h>
#include "proto.h"

// No message for this long means "Mukk offline" (Mukk sends state every 5 s).
#define PLAYER_TIMEOUT_MS 15000

typedef struct {
    bool session;        // a Mukk client is connected
    int64_t last_msg_ms; // last message (or the connect) of the session

    bool has_track; // false: nothing loaded, or no track message yet
    track_t track;
    bool has_next;
    next_t next;
    int64_t track_since_ms; // when the current track id first showed up

    bool has_state; // false until the first state message
    play_status_t status;
    int32_t position_ms;    // as last reported...
    int64_t position_at_ms; // ...at this time
    int volume;
    repeat_t repeat;
    bool shuffle;
} player_t;

// Starts with no session.
void player_init(player_t *p);

// A new client connected: forget the old session, Mukk resends everything.
void player_session_start(player_t *p, int64_t now_ms);

// The client went away.
void player_session_end(player_t *p);

void player_apply(player_t *p, const proto_msg_t *msg, int64_t now_ms);

// Connected and heard from within PLAYER_TIMEOUT_MS.
bool player_online(const player_t *p, int64_t now_ms);

// The reported position, moved on by the time since while playing, and
// clamped to the duration when that is known.
int32_t player_position_ms(const player_t *p, int64_t now_ms);
