#include <math.h>
#include <stdio.h>
#include "ui.h"
#include "font.h"

#define SCROLL_HOLD_MS  2000
#define SCROLL_PX_PER_S 30

// Now-playing layout, top edges in px: the cover, three text lines, the
// progress bar, then status icon, position, volume and duration along the
// bottom. The panel's corners are rounded, hence the margins.
#define COVER_SIZE PROTO_COVER_SIZE
#define Y_COVER    8
#define Y_TITLE    174
#define Y_ARTIST   200
#define Y_ALBUM    220
#define Y_BAR      246
#define BAR_H      4
#define Y_BOTTOM   255
#define MARGIN     12 // text lines
#define MARGIN_BOT 18 // bottom row, closer to the rounded corners
#define ICON_H     12

#define C_BG     RGB(0, 0, 0)
#define C_TITLE  RGB(255, 255, 255)
#define C_ARTIST RGB(210, 210, 210)
#define C_DIM    RGB(140, 140, 140)
#define C_COVER  RGB(40, 40, 40) // the cover's place while there's no art
#define C_TRACK  RGB(60, 60, 60) // progress bar background
#define C_ACCENT RGB(255, 150, 40)

void ui_format_time(char *buf, size_t len, int32_t ms)
{
    int s = ms / 1000;
    if (s >= 3600) {
        snprintf(buf, len, "%d:%02d:%02d", s / 3600, s / 60 % 60, s % 60);
    } else {
        snprintf(buf, len, "%d:%02d", s / 60, s % 60);
    }
}

int ui_scroll_offset(int text_w, int box_w, int64_t t_ms)
{
    if (text_w <= box_w || t_ms < 0) return 0;
    int travel = text_w - box_w;
    int64_t scroll_ms = (int64_t)travel * 1000 / SCROLL_PX_PER_S;
    int64_t u = t_ms % (SCROLL_HOLD_MS + scroll_ms + SCROLL_HOLD_MS);
    if (u < SCROLL_HOLD_MS) return 0;
    if (u < SCROLL_HOLD_MS + scroll_ms) return (int)((u - SCROLL_HOLD_MS) * SCROLL_PX_PER_S / 1000);
    return travel;
}

// A line that is centred when it fits and scrolls within the margins when
// it doesn't. Empty text draws nothing.
static void scroll_line(canvas_t *c, const font_t *font, int y, const char *text, uint16_t color,
                        int64_t t_ms)
{
    int w = font_text_width(font, text);
    int box = CANVAS_W - 2 * MARGIN;
    if (w <= box) {
        canvas_text_center(c, font, y, text, color);
        return;
    }
    canvas_text_clipped(c, font, MARGIN - ui_scroll_offset(w, box, t_ms), y, text, color, MARGIN,
                        CANVAS_W - MARGIN);
}

// Waiting screen: two bored eyes above the text. They look around, blink,
// roll now and then and get sleepier, nod off, sleep (zzz), wake with a
// start and begin again. All a function of the time since the screen came
// up, like everything else here.
#define EYE_Y      62
#define EYE_DX     42 // eye centre to the middle of the screen
#define EYE_RX     30
#define EYE_RY     38
#define IRIS_R     14
#define PUPIL_R    7
#define LOOK_X     (EYE_RX - IRIS_R - 3) // how far the iris can move and stay inside
#define LOOK_Y     (EYE_RY - IRIS_R - 3)
#define BEAT_MS    1800 // one look (or eye roll) per beat
#define GLANCE_MS  250  // moving to the next look
#define ROLL_MS    1000
#define BLINK_AT   1000 // into the beat
#define BLINK_MS   180
#define LID_BORED  0.2f // resting lid level, 0 open .. 1 closed
#define LID_DROOPY 0.6f

// The mood cycle, then it repeats.
#define BORED_MS 20000
#define DROOP_MS 60000 // lids sink from bored to droopy
#define NOD_MS   6000  // three nods, each deeper, the last one closes them
#define SLEEP_MS 25000
#define JOLT_MS  3000 // wide awake, looking around
#define CYCLE_MS (BORED_MS + DROOP_MS + NOD_MS + SLEEP_MS + JOLT_MS)
#define ZZZ_MS   3000 // one z rising and fading

#define C_SCLERA   RGB(235, 235, 235)
#define C_LID      RGB(70, 70, 80)
#define C_LID_EDGE RGB(150, 150, 160)

typedef struct {
    float lid; // resting level, blinks come on top
    enum { MOOD_AWAKE, MOOD_ASLEEP, MOOD_JOLT } kind;
    int64_t t; // ms into the asleep or jolt phase
} mood_t;

static mood_t eyes_mood(int64_t t)
{
    t %= CYCLE_MS;
    if (t < BORED_MS) return (mood_t){LID_BORED, MOOD_AWAKE, 0};
    t -= BORED_MS;
    if (t < DROOP_MS)
        return (mood_t){LID_BORED + (LID_DROOPY - LID_BORED) * (float)t / DROOP_MS, MOOD_AWAKE, 0};
    t -= DROOP_MS;
    if (t < NOD_MS) {
        // Sinks slowly, snaps back up; deeper each time.
        int seg = NOD_MS / 3;
        int nod = (int)(t / seg);
        float u = (float)(t % seg) / (float)seg;
        float depth = (float)(nod + 1) / 3;
        return (mood_t){LID_DROOPY + (1 - LID_DROOPY) * u * u * depth, MOOD_AWAKE, 0};
    }
    t -= NOD_MS;
    if (t < SLEEP_MS) return (mood_t){1, MOOD_ASLEEP, t};
    return (mood_t){0, MOOD_JOLT, t - SLEEP_MS};
}

static uint32_t hash32(uint32_t x)
{
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

// Where the eyes look, in units of LOOK_X/LOOK_Y. Bored: mostly ahead or down.
static const float LOOKS[][2] = {
    {0, 0}, {0, 0.6f}, {-1, 0}, {1, 0}, {-0.7f, 0.7f}, {0.7f, 0.7f}, {0, -1}, {-0.7f, -0.7f}, {0.7f, -0.7f},
};
#define N_LOOKS ((int)(sizeof(LOOKS) / sizeof(LOOKS[0])))

static bool beat_rolls(uint32_t beat)
{
    return beat > 0 && hash32(beat) % 8 == 0;
}

// Where a beat's look ends up. An eye roll goes left, over the top, right.
static void beat_look(uint32_t beat, float *x, float *y)
{
    if (beat_rolls(beat)) {
        *x = 1, *y = 0;
        return;
    }
    const float *l = LOOKS[beat == 0 ? 0 : (hash32(beat) >> 4) % N_LOOKS];
    *x = l[0], *y = l[1];
}

static float smooth(float u)
{
    return u <= 0 ? 0 : u >= 1 ? 1 : u * u * (3 - 2 * u);
}

static void eyes_look(int64_t t, float *x, float *y)
{
    uint32_t beat = (uint32_t)(t / BEAT_MS);
    int64_t u = t % BEAT_MS;
    if (beat_rolls(beat) && u < ROLL_MS) {
        float a = (float)M_PI * smooth((float)u / ROLL_MS);
        *x = -cosf(a), *y = -sinf(a);
        return;
    }
    float x0;
    float y0;
    float x1;
    float y1;
    beat_look(beat == 0 ? 0 : beat - 1, &x0, &y0);
    beat_look(beat, &x1, &y1);
    if (beat_rolls(beat)) x0 = x1, y0 = y1; // the roll already ended there
    float k = smooth((float)u / GLANCE_MS);
    *x = x0 + (x1 - x0) * k, *y = y0 + (y1 - y0) * k;
}

// How far the lids are closed by a blink, 0..1.
static float eyes_blink(int64_t t)
{
    uint32_t beat = (uint32_t)(t / BEAT_MS);
    int64_t u = t % BEAT_MS - BLINK_AT;
    if (beat == 0 || beat_rolls(beat) || (hash32(beat) >> 12) % 3 != 0 || u < 0 || u >= BLINK_MS) return 0;
    return 1 - fabsf(2.0f * (float)u / BLINK_MS - 1);
}

static int lid_row(float lid)
{
    return (int)(EYE_Y - EYE_RY + lid * 2 * EYE_RY + 0.5f);
}

// One eye; lids 0 open .. 1 closed, look in units of LOOK_X/LOOK_Y. Below
// drooping lids the iris looks down with them rather than hide.
static void eye(canvas_t *c, float cx, float rest_lid, float lid, float look_x, float look_y)
{
    canvas_ellipse(c, cx, EYE_Y, EYE_RX, EYE_RY, C_SCLERA, 0, CANVAS_H);
    float ix = cx + look_x * LOOK_X;
    float iy = EYE_Y + look_y * LOOK_Y;
    float below_lid = (float)lid_row(rest_lid) + 2;
    if (iy < below_lid) iy = below_lid < EYE_Y + LOOK_Y ? below_lid : EYE_Y + LOOK_Y;
    canvas_ellipse(c, ix, iy, IRIS_R, IRIS_R, C_ACCENT, 0, CANVAS_H);
    canvas_ellipse(c, ix, iy, PUPIL_R, PUPIL_R, C_BG, 0, CANVAS_H);
    canvas_ellipse(c, ix - 4, iy - 4, 2.5f, 2.5f, C_SCLERA, 0, CANVAS_H); // glint
    if (lid <= 0) return;
    int lid_y = lid_row(lid);
    // A size larger, so no white shows at the anti-aliased rim.
    canvas_ellipse(c, cx, EYE_Y, EYE_RX + 1, EYE_RY + 1, C_LID, 0, lid_y);
    canvas_ellipse(c, cx, EYE_Y, EYE_RX + 1, EYE_RY + 1, C_LID_EDGE, lid_y - 2, lid_y);
}

// Three z's rising from the right eye, one after another, fading out.
static void zzz(canvas_t *c, int64_t t)
{
    for (int i = 0; i < 3; i++) {
        int64_t u = t - i * ZZZ_MS / 3;
        if (u < 0) continue;
        float f = (float)(u % ZZZ_MS) / ZZZ_MS;
        int x = CANVAS_W / 2 + EYE_DX + EYE_RX + 2 + (int)(f * 14 + 3 * sinf(f * 2 * (float)M_PI));
        int y = EYE_Y - 12 - (int)(f * 40);
        int g = (int)(230 * (1 - f));
        canvas_text(c, f < 0.4f ? &FONT_TEXT : &FONT_TITLE, x, y, f < 0.4f ? "z" : "Z", RGB(g, g, g));
    }
}

static void eyes(canvas_t *c, int64_t t)
{
    if (t < 0) t = 0;
    mood_t m = eyes_mood(t);
    float lid = m.lid;
    float x = 0;
    float y = 0;
    if (m.kind == MOOD_AWAKE) {
        eyes_look(t, &x, &y);
        float b = eyes_blink(t);
        lid += (1 - lid) * b;
    } else if (m.kind == MOOD_JOLT) {
        // Darting left and right, calming down to ahead.
        float calm = 1 - (float)m.t / JOLT_MS;
        x = sinf((float)m.t * 2 * (float)M_PI / 700) * calm;
    }
    eye(c, CANVAS_W / 2.0f - EYE_DX, m.lid, lid, x, y);
    eye(c, CANVAS_W / 2.0f + EYE_DX, m.lid, lid, x, y);
    if (m.kind == MOOD_ASLEEP) zzz(c, m.t);
}

static void message(canvas_t *c, const char *l1, const char *l2, const char *l3)
{
    canvas_text_center(c, &FONT_TITLE, 80, "Mukklet", C_TITLE);
    canvas_fill(c, 40, 110, CANVAS_W - 80, 1, C_DIM);
    if (l1) canvas_text_center(c, &FONT_TEXT, 124, l1, C_ARTIST);
    if (l2) canvas_text_center(c, &FONT_TEXT, 148, l2, C_DIM);
    if (l3) canvas_text_center(c, &FONT_TEXT, 170, l3, C_DIM);
}

// Play triangle, pause bars or stop square, ICON_H tall, top-left at (x, y).
static void status_icon(canvas_t *c, int x, int y, play_status_t status)
{
    if (status == PLAY_PLAYING) {
        for (int i = 0; i < ICON_H; i++) {
            int half = i < ICON_H / 2 ? i : ICON_H - 1 - i;
            canvas_fill(c, x, y + i, 2 + half * 2, 1, C_DIM);
        }
    } else if (status == PLAY_PAUSED) {
        canvas_fill(c, x, y, 4, ICON_H, C_DIM);
        canvas_fill(c, x + 7, y, 4, ICON_H, C_DIM);
    } else {
        canvas_fill(c, x + 1, y + 1, ICON_H - 2, ICON_H - 2, C_DIM);
    }
}

static void bottom_row(canvas_t *c, const player_t *p, int64_t now_ms)
{
    if (!p->has_state) return;
    int text_top = Y_BOTTOM + (FONT_TEXT.ascent - ICON_H) - 1; // icon sits on the baseline
    status_icon(c, MARGIN_BOT, text_top, p->status);

    char buf[16];
    if (p->has_track) {
        ui_format_time(buf, sizeof(buf), player_position_ms(p, now_ms));
        canvas_text(c, &FONT_TEXT, MARGIN_BOT + ICON_H + 5, Y_BOTTOM, buf, C_DIM);
        if (p->track.duration_ms > 0) {
            ui_format_time(buf, sizeof(buf), p->track.duration_ms);
            canvas_text_right(c, &FONT_TEXT, CANVAS_W - MARGIN_BOT, Y_BOTTOM, buf, C_DIM);
        }
    }
    snprintf(buf, sizeof(buf), "%d%%", p->volume);
    canvas_text_center(c, &FONT_TEXT, Y_BOTTOM, buf, C_DIM);
}

static void progress_bar(canvas_t *c, const player_t *p, int64_t now_ms)
{
    int w = CANVAS_W - 2 * MARGIN;
    canvas_fill(c, MARGIN, Y_BAR, w, BAR_H, C_TRACK);
    if (!p->has_state || p->track.duration_ms <= 0) return;
    int done = (int)((int64_t)player_position_ms(p, now_ms) * w / p->track.duration_ms);
    canvas_fill(c, MARGIN, Y_BAR, done, BAR_H, C_ACCENT);
}

static void now_playing(canvas_t *c, const player_t *p, const uint8_t *cover, int64_t now_ms)
{
    int cover_x = (CANVAS_W - COVER_SIZE) / 2;
    if (cover) {
        canvas_image_be(c, cover_x, Y_COVER, COVER_SIZE, COVER_SIZE, cover);
    } else {
        canvas_fill(c, cover_x, Y_COVER, COVER_SIZE, COVER_SIZE, C_COVER);
    }

    int64_t t = now_ms - p->track_since_ms;
    scroll_line(c, &FONT_TITLE, Y_TITLE, p->track.title, C_TITLE, t);
    scroll_line(c, &FONT_TEXT, Y_ARTIST, p->track.artist, C_ARTIST, t);

    char line[PROTO_TEXT_MAX + 16]; // + " (year)"
    if (p->track.year > 0 && p->track.album[0]) {
        snprintf(line, sizeof(line), "%s (%d)", p->track.album, p->track.year);
    } else if (p->track.year > 0) {
        snprintf(line, sizeof(line), "%d", p->track.year);
    } else {
        snprintf(line, sizeof(line), "%s", p->track.album);
    }
    scroll_line(c, &FONT_TEXT, Y_ALBUM, line, C_DIM, t);

    progress_bar(c, p, now_ms);
    bottom_row(c, p, now_ms);
}

ui_screen_t ui_screen(const ui_input_t *in, int64_t now_ms)
{
    const player_t *p = in->player;
    if (!in->wifi) return UI_CONNECTING;
    if (!p->session) return UI_WAITING;
    if (!player_online(p, now_ms)) return UI_OFFLINE;
    return p->has_track ? UI_PLAYING : UI_NOTHING;
}

void ui_render(canvas_t *c, const ui_input_t *in, int64_t now_ms)
{
    canvas_fill(c, 0, 0, CANVAS_W, CANVAS_H, C_BG);
    const player_t *p = in->player;
    switch (ui_screen(in, now_ms)) {
        case UI_CONNECTING: message(c, "Connecting to WiFi", NULL, NULL); break;
        case UI_WAITING:
            eyes(c, now_ms - in->screen_since_ms);
            canvas_text_center(c, &FONT_TEXT, 124, "Waiting for Mukk", C_ARTIST);
            canvas_text_center(c, &FONT_TEXT, 148, "mukklet.local", C_DIM);
            if (in->ip) canvas_text_center(c, &FONT_TEXT, 170, in->ip, C_DIM);
            break;
        case UI_OFFLINE: message(c, "Mukk offline", NULL, NULL); break;
        case UI_NOTHING:
            eyes(c, now_ms - in->screen_since_ms);
            canvas_text_center(c, &FONT_TEXT, 124, "Nothing playing", C_ARTIST);
            bottom_row(c, p, now_ms);
            break;
        case UI_PLAYING: now_playing(c, p, in->cover, now_ms); break;
    }
}
