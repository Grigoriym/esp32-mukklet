#include <stdio.h>
#include <string.h>
#include "proto.h"
#include "cJSON.h"

// Copies a JSON string field, cut to fit at a UTF-8 character boundary;
// "" when missing or not a string.
static void copy_str(char *dst, size_t size, const cJSON *obj, const char *key)
{
    const char *s = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(obj, key));
    if (!s) s = "";
    size_t n = strlen(s);
    if (n >= size) {
        n = size - 1;
        while (n > 0 && ((unsigned char)s[n] & 0xC0) == 0x80) n--; // don't split a character
    }
    memcpy(dst, s, n);
    dst[n] = '\0';
}

static double get_num(const cJSON *obj, const char *key)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(obj, key);
    return cJSON_IsNumber(item) ? item->valuedouble : 0;
}

static int32_t get_ms(const cJSON *obj, const char *key)
{
    double v = get_num(obj, key);
    if (v < 0) return 0;
    return v > INT32_MAX ? INT32_MAX : (int32_t)v;
}

static bool is_str(const cJSON *obj, const char *key, const char *value)
{
    const char *s = cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(obj, key));
    return s && strcmp(s, value) == 0;
}

static void parse_track(const cJSON *root, proto_msg_t *out)
{
    const cJSON *t = cJSON_GetObjectItemCaseSensitive(root, "track");
    out->track.present = cJSON_IsObject(t);
    if (out->track.present) {
        track_t *tr = &out->track.track;
        copy_str(tr->id, sizeof(tr->id), t, "id");
        copy_str(tr->title, sizeof(tr->title), t, "title");
        copy_str(tr->artist, sizeof(tr->artist), t, "artist");
        copy_str(tr->album, sizeof(tr->album), t, "album");
        tr->year = (int)get_num(t, "year");
        tr->duration_ms = get_ms(t, "durationMs");
    }
    const cJSON *n = cJSON_GetObjectItemCaseSensitive(root, "next");
    out->track.has_next = cJSON_IsObject(n);
    if (out->track.has_next) {
        copy_str(out->track.next.title, sizeof(out->track.next.title), n, "title");
        copy_str(out->track.next.artist, sizeof(out->track.next.artist), n, "artist");
    }
}

static void parse_state(const cJSON *root, proto_msg_t *out)
{
    if (is_str(root, "status", "playing")) {
        out->state.status = PLAY_PLAYING;
    } else if (is_str(root, "status", "paused")) {
        out->state.status = PLAY_PAUSED;
    } else if (is_str(root, "status", "stopped")) {
        out->state.status = PLAY_STOPPED;
    } else {
        out->state.status = PLAY_IDLE;
    }
    out->state.position_ms = get_ms(root, "positionMs");
    double vol = get_num(root, "volume");
    out->state.volume = vol < 0 ? 0 : vol > 100 ? 100 : (int)(vol + 0.5);
    if (is_str(root, "repeat", "one")) {
        out->state.repeat = REPEAT_ONE;
    } else if (is_str(root, "repeat", "all")) {
        out->state.repeat = REPEAT_ALL;
    } else {
        out->state.repeat = REPEAT_OFF;
    }
    out->state.shuffle = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "shuffle"));
}

bool proto_parse(const char *json, size_t len, proto_msg_t *out)
{
    memset(out, 0, sizeof(*out));
    cJSON *root = cJSON_ParseWithLength(json, len);
    if (!cJSON_IsObject(root) || !cJSON_IsString(cJSON_GetObjectItemCaseSensitive(root, "type"))) {
        cJSON_Delete(root);
        return false;
    }
    if (is_str(root, "type", "track")) {
        out->type = MSG_TRACK;
        parse_track(root, out);
    } else if (is_str(root, "type", "state")) {
        out->type = MSG_STATE;
        parse_state(root, out);
    } else {
        out->type = MSG_OTHER;
    }
    cJSON_Delete(root);
    return true;
}

static int fitted(int n, size_t len)
{
    return (n < 0 || (size_t)n >= len) ? -1 : n;
}

int proto_hello(char *buf, size_t len)
{
    return fitted(snprintf(buf, len,
                           "{\"type\":\"hello\",\"v\":1,\"device\":\"mukklet-oled\","
                           "\"cover\":{\"w\":64,\"h\":64,\"format\":\"none\"},\"maxChunk\":4096}"),
                  len);
}

int proto_cmd(char *buf, size_t len, cmd_t cmd, int arg)
{
    int n;
    switch (cmd) {
        case CMD_PLAY_PAUSE: n = snprintf(buf, len, "{\"type\":\"cmd\",\"cmd\":\"play_pause\"}"); break;
        case CMD_NEXT: n = snprintf(buf, len, "{\"type\":\"cmd\",\"cmd\":\"next\"}"); break;
        case CMD_PREV: n = snprintf(buf, len, "{\"type\":\"cmd\",\"cmd\":\"prev\"}"); break;
        case CMD_VOLUME:
            n = snprintf(buf, len, "{\"type\":\"cmd\",\"cmd\":\"volume\",\"delta\":%d}", arg);
            break;
        case CMD_SEEK:
            n = snprintf(buf, len, "{\"type\":\"cmd\",\"cmd\":\"seek\",\"deltaMs\":%d}", arg);
            break;
        default: return -1;
    }
    return fitted(n, len);
}
