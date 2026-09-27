#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "proto.h"
#include "cJSON.h"

static proto_msg_t msg;

void setUp(void)
{
}

void tearDown(void)
{
}

// The examples in docs/protocol/ are the contract; test against them as is.
static char *read_example(const char *name)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", PROTOCOL_DIR, name);
    FILE *f = fopen(path, "rb");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, path);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(n + 1);
    TEST_ASSERT_EQUAL(n, (long)fread(buf, 1, n, f));
    buf[n] = '\0';
    fclose(f);
    return buf;
}

static bool parse_str(const char *json)
{
    return proto_parse(json, strlen(json), &msg);
}

static void test_track_example(void)
{
    char *json = read_example("track.json");
    TEST_ASSERT_TRUE(parse_str(json));
    free(json);
    TEST_ASSERT_EQUAL(MSG_TRACK, msg.type);
    TEST_ASSERT_TRUE(msg.track.present);
    TEST_ASSERT_EQUAL_STRING("a3f9c2", msg.track.track.id);
    TEST_ASSERT_EQUAL_STRING("Paranoid Android", msg.track.track.title);
    TEST_ASSERT_EQUAL_STRING("Radiohead", msg.track.track.artist);
    TEST_ASSERT_EQUAL_STRING("OK Computer", msg.track.track.album);
    TEST_ASSERT_EQUAL(1997, msg.track.track.year);
    TEST_ASSERT_EQUAL(386000, msg.track.track.duration_ms);
    TEST_ASSERT_TRUE(msg.track.has_next);
    TEST_ASSERT_EQUAL_STRING("Subterranean Homesick Alien", msg.track.next.title);
}

static void test_track_none_example(void)
{
    char *json = read_example("track_none.json");
    TEST_ASSERT_TRUE(parse_str(json));
    free(json);
    TEST_ASSERT_EQUAL(MSG_TRACK, msg.type);
    TEST_ASSERT_FALSE(msg.track.present);
    TEST_ASSERT_FALSE(msg.track.has_next);
}

static void test_state_example(void)
{
    char *json = read_example("state.json");
    TEST_ASSERT_TRUE(parse_str(json));
    free(json);
    TEST_ASSERT_EQUAL(MSG_STATE, msg.type);
    TEST_ASSERT_EQUAL(PLAY_PLAYING, msg.state.status);
    TEST_ASSERT_EQUAL(125340, msg.state.position_ms);
    TEST_ASSERT_EQUAL(80, msg.state.volume);
    TEST_ASSERT_EQUAL(REPEAT_ALL, msg.state.repeat);
    TEST_ASSERT_FALSE(msg.state.shuffle);
}

static void test_cover_and_unknown_types_ignored(void)
{
    char *json = read_example("cover_none.json");
    TEST_ASSERT_TRUE(parse_str(json));
    free(json);
    TEST_ASSERT_EQUAL(MSG_OTHER, msg.type);
    TEST_ASSERT_TRUE(parse_str("{\"type\":\"lyrics\",\"text\":\"la\"}"));
    TEST_ASSERT_EQUAL(MSG_OTHER, msg.type);
}

static void test_not_protocol(void)
{
    TEST_ASSERT_FALSE(parse_str("not json"));
    TEST_ASSERT_FALSE(parse_str("[1,2]"));
    TEST_ASSERT_FALSE(parse_str("{\"title\":\"x\"}"));
    TEST_ASSERT_FALSE(parse_str("{\"type\":7}"));
    TEST_ASSERT_FALSE(parse_str(""));
}

static void test_length_bounds_the_input(void)
{
    // Frames aren't NUL-terminated; only len bytes count.
    const char *json = "{\"type\":\"state\",\"volume\":40}GARBAGE";
    TEST_ASSERT_TRUE(proto_parse(json, strlen(json) - 7, &msg));
    TEST_ASSERT_EQUAL(40, msg.state.volume);
}

static void test_missing_and_odd_fields(void)
{
    TEST_ASSERT_TRUE(
        parse_str("{\"type\":\"track\",\"track\":{\"id\":\"x\",\"title\":\"T\",\"year\":\"1997\","
                  "\"durationMs\":-5,\"extra\":{}}}"));
    TEST_ASSERT_EQUAL_STRING("T", msg.track.track.title);
    TEST_ASSERT_EQUAL_STRING("", msg.track.track.artist);
    TEST_ASSERT_EQUAL(0, msg.track.track.year); // a string isn't a number
    TEST_ASSERT_EQUAL(0, msg.track.track.duration_ms);
    TEST_ASSERT_FALSE(msg.track.has_next); // missing = null

    TEST_ASSERT_TRUE(parse_str("{\"type\":\"state\",\"status\":\"buffering\",\"volume\":140.0}"));
    TEST_ASSERT_EQUAL(PLAY_IDLE, msg.state.status);
    TEST_ASSERT_EQUAL(100, msg.state.volume);
    TEST_ASSERT_EQUAL(REPEAT_OFF, msg.state.repeat);
}

static void test_long_text_cut_at_character_boundary(void)
{
    // 200 two-byte characters: cut to fit, never mid-character.
    char json[1024] = "{\"type\":\"track\",\"track\":{\"title\":\"";
    for (int i = 0; i < 200; i++) strcat(json, "Ж");
    strcat(json, "\"}}");
    TEST_ASSERT_TRUE(parse_str(json));
    size_t n = strlen(msg.track.track.title);
    TEST_ASSERT_TRUE(n < PROTO_TEXT_MAX);
    TEST_ASSERT_EQUAL(0, n % 2);
    TEST_ASSERT_EQUAL(PROTO_TEXT_MAX - 2, n); // 158 bytes = 79 characters
}

static void test_hello(void)
{
    char buf[256];
    int n = proto_hello(buf, sizeof(buf));
    TEST_ASSERT_EQUAL((int)strlen(buf), n);
    cJSON *j = cJSON_Parse(buf);
    TEST_ASSERT_EQUAL_STRING("hello", cJSON_GetStringValue(cJSON_GetObjectItem(j, "type")));
    TEST_ASSERT_EQUAL(1, cJSON_GetNumberValue(cJSON_GetObjectItem(j, "v")));
    const cJSON *cover = cJSON_GetObjectItem(j, "cover");
    TEST_ASSERT_EQUAL_STRING("none", cJSON_GetStringValue(cJSON_GetObjectItem(cover, "format")));
    TEST_ASSERT_EQUAL(4096, cJSON_GetNumberValue(cJSON_GetObjectItem(j, "maxChunk")));
    cJSON_Delete(j);
    TEST_ASSERT_EQUAL(-1, proto_hello(buf, 20));
}

// Every command must match docs/protocol/cmd.json field for field.
static void test_cmds_match_example(void)
{
    char *json = read_example("cmd.json");
    cJSON *examples = cJSON_Parse(json);
    free(json);
    struct {
        cmd_t cmd;
        int arg;
    } cases[] = {{CMD_PLAY_PAUSE, 0}, {CMD_NEXT, 0}, {CMD_PREV, 0}, {CMD_VOLUME, 5}, {CMD_SEEK, -10000}};
    TEST_ASSERT_EQUAL(5, cJSON_GetArraySize(examples));
    for (int i = 0; i < 5; i++) {
        char buf[96];
        TEST_ASSERT_TRUE(proto_cmd(buf, sizeof(buf), cases[i].cmd, cases[i].arg) > 0);
        cJSON *built = cJSON_Parse(buf);
        TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(cJSON_GetArrayItem(examples, i), built, true), buf);
        cJSON_Delete(built);
    }
    cJSON_Delete(examples);
    char tiny[8];
    TEST_ASSERT_EQUAL(-1, proto_cmd(tiny, sizeof(tiny), CMD_NEXT, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_track_example);
    RUN_TEST(test_track_none_example);
    RUN_TEST(test_state_example);
    RUN_TEST(test_cover_and_unknown_types_ignored);
    RUN_TEST(test_not_protocol);
    RUN_TEST(test_length_bounds_the_input);
    RUN_TEST(test_missing_and_odd_fields);
    RUN_TEST(test_long_text_cut_at_character_boundary);
    RUN_TEST(test_hello);
    RUN_TEST(test_cmds_match_example);
    return UNITY_END();
}
