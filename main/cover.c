#include <string.h>
#include "cover.h"

void cover_init(cover_t *c, uint8_t *buf)
{
    memset(c, 0, sizeof(*c));
    c->px = buf;
}

void cover_reset(cover_t *c)
{
    cover_init(c, c->px);
}

void cover_begin(cover_t *c, const proto_msg_t *msg)
{
    memcpy(c->track_id, msg->cover.track_id, sizeof(c->track_id));
    c->complete = false; // the old art is overwritten from here on
    c->got = 0;
    c->receiving = !msg->cover.none && msg->cover.rgb565 && msg->cover.w == PROTO_COVER_SIZE
                   && msg->cover.h == PROTO_COVER_SIZE && msg->cover.size == PROTO_COVER_BYTES;
}

uint8_t *cover_chunk_dst(const cover_t *c, size_t n)
{
    if (!c->receiving || n == 0 || n > PROTO_MAX_CHUNK || n > PROTO_COVER_BYTES - c->got) return NULL;
    return c->px + c->got;
}

bool cover_chunk_done(cover_t *c, size_t n)
{
    if (!c->receiving) return false;
    c->got += n;
    if (c->got < PROTO_COVER_BYTES) return false;
    c->receiving = false;
    c->complete = true;
    return true;
}

void cover_cancel(cover_t *c)
{
    if (!c->receiving) return; // a finished cover stays
    c->receiving = false;
    c->got = 0;
}

const uint8_t *cover_for(const cover_t *c, const char *track_id)
{
    return c->complete && strcmp(c->track_id, track_id) == 0 ? c->px : NULL;
}
