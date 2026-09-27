#include <string.h>
#include "player.h"

void player_init(player_t *p)
{
    memset(p, 0, sizeof(*p));
}

void player_session_start(player_t *p, int64_t now_ms)
{
    player_init(p);
    p->session = true;
    p->last_msg_ms = now_ms;
}

void player_session_end(player_t *p)
{
    player_init(p);
}

void player_apply(player_t *p, const proto_msg_t *msg, int64_t now_ms)
{
    p->last_msg_ms = now_ms;
    switch (msg->type) {
        case MSG_TRACK:
            // A repeated message for the same track (Mukk resends after a
            // command) mustn't restart the scrolling.
            if (!msg->track.present || !p->has_track || strcmp(p->track.id, msg->track.track.id) != 0) {
                p->track_since_ms = now_ms;
            }
            p->has_track = msg->track.present;
            p->track = msg->track.track;
            p->has_next = msg->track.has_next;
            p->next = msg->track.next;
            break;
        case MSG_STATE:
            p->has_state = true;
            p->status = msg->state.status;
            p->position_ms = msg->state.position_ms;
            p->position_at_ms = now_ms;
            p->volume = msg->state.volume;
            p->repeat = msg->state.repeat;
            p->shuffle = msg->state.shuffle;
            break;
        default: break; // still counts as a heartbeat
    }
}

bool player_online(const player_t *p, int64_t now_ms)
{
    return p->session && now_ms - p->last_msg_ms < PLAYER_TIMEOUT_MS;
}

int32_t player_position_ms(const player_t *p, int64_t now_ms)
{
    int64_t pos = p->position_ms;
    if (p->status == PLAY_PLAYING && now_ms > p->position_at_ms) pos += now_ms - p->position_at_ms;
    if (p->has_track && p->track.duration_ms > 0 && pos > p->track.duration_ms) pos = p->track.duration_ms;
    return pos > INT32_MAX ? INT32_MAX : (int32_t)pos;
}
