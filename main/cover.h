#pragma once

// Assembles the cover art from a `cover` message and the binary frames
// after it (docs/PROTOCOL.md) into one PROTO_COVER_BYTES buffer, and says
// which track the finished art belongs to. One buffer only (51 KB, no
// PSRAM): while a new cover arrives there is no art to show. Pure, so it
// also compiles on the PC for the unit tests in test/.
//
// The frames are received straight into the buffer: cover_chunk_dst() says
// where, cover_chunk_done() counts them in. Nothing reads the pixels while
// cover_for() returns NULL, so only the calls that change its answer
// (begin, done, reset) need a lock against the reader.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "proto.h"

typedef struct {
    uint8_t *px;                 // PROTO_COVER_BYTES, RGB565 big-endian
    char track_id[PROTO_ID_MAX]; // whose art px holds (when complete)
    bool complete;
    bool receiving; // a cover message came, its frames are due
    size_t got;     // bytes of it so far
} cover_t;

// No art; buf is PROTO_COVER_BYTES and stays the cover's.
void cover_init(cover_t *c, uint8_t *buf);

// No art (a new session), same buffer.
void cover_reset(cover_t *c);

// A cover message. Art in the size and format hello asked for starts
// receiving; "none" or anything else leaves no art.
void cover_begin(cover_t *c, const proto_msg_t *msg);

// Where a binary frame of n bytes goes, or NULL if none is due or it
// doesn't fit (then call cover_cancel()).
uint8_t *cover_chunk_dst(const cover_t *c, size_t n);

// n bytes were written to cover_chunk_dst(). True when that completed it.
bool cover_chunk_done(cover_t *c, size_t n);

// A partly received cover is dropped: a text message came in between (the
// protocol never interleaves), a frame didn't fit or failed.
void cover_cancel(cover_t *c);

// The finished art for this track, or NULL.
const uint8_t *cover_for(const cover_t *c, const char *track_id);
