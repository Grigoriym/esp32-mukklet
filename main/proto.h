#pragma once

// docs/PROTOCOL.md messages: parsing what Mukk sends, building what the
// display sends. Pure (cJSON only), so it also compiles on the PC for the
// unit tests in test/.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Tag strings longer than these are cut (at a UTF-8 boundary); far more than
// fits on screen even when scrolling.
#define PROTO_ID_MAX   48
#define PROTO_TEXT_MAX 160

// The cover art hello asks for: square, RGB565 big-endian, in binary frames
// of at most PROTO_MAX_CHUNK bytes.
#define PROTO_COVER_SIZE  160
#define PROTO_COVER_BYTES (PROTO_COVER_SIZE * PROTO_COVER_SIZE * 2)
#define PROTO_MAX_CHUNK   4096

typedef enum {
    PLAY_IDLE,
    PLAY_STOPPED,
    PLAY_PAUSED,
    PLAY_PLAYING,
} play_status_t;

typedef enum {
    REPEAT_OFF,
    REPEAT_ONE,
    REPEAT_ALL,
} repeat_t;

typedef struct {
    char id[PROTO_ID_MAX];
    char title[PROTO_TEXT_MAX];
    char artist[PROTO_TEXT_MAX];
    char album[PROTO_TEXT_MAX];
    int year;            // 0 = unknown
    int32_t duration_ms; // 0 = unknown
} track_t;

typedef struct {
    char title[PROTO_TEXT_MAX];
    char artist[PROTO_TEXT_MAX];
} next_t;

typedef enum {
    MSG_OTHER, // valid, but nothing the display uses (unknown types)
    MSG_TRACK,
    MSG_STATE,
    MSG_COVER,
} msg_type_t;

typedef struct {
    msg_type_t type;
    union {
        struct {
            bool present; // false: "track": null, nothing loaded
            track_t track;
            bool has_next; // false: "next": null, not known
            next_t next;
        } track;
        struct {
            play_status_t status;
            int32_t position_ms;
            int volume; // 0-100
            repeat_t repeat;
            bool shuffle;
        } state;
        struct {
            char track_id[PROTO_ID_MAX];
            bool none;   // "none": true, no art for this track
            int w, h;    // 0 when missing
            bool rgb565; // format "rgb565"; anything else is of no use here
            int32_t size;
        } cover;
    };
} proto_msg_t;

// Parses one text frame (len bytes, needn't be NUL-terminated). Missing or
// mistyped fields get their "unknown" value, unknown fields and types are
// ignored (MSG_OTHER). False only when it isn't a JSON object with a string
// "type".
bool proto_parse(const char *json, size_t len, proto_msg_t *out);

typedef enum {
    CMD_PLAY_PAUSE,
    CMD_NEXT,
    CMD_PREV,
    CMD_VOLUME, // arg = delta in percentage points
    CMD_SEEK,   // arg = delta in ms
} cmd_t;

// The "hello" this display sends on connect, asking for PROTO_COVER_SIZE
// square rgb565 covers. Returns the length, or -1 if buf is too small.
int proto_hello(char *buf, size_t len);

// A "cmd" message. Returns the length, or -1 if buf is too small.
int proto_cmd(char *buf, size_t len, cmd_t cmd, int arg);
