#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "link.h"
#include "cover.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "mdns.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "link";

#define MDNS_HOSTNAME "mukklet" // -> ws://mukklet.local/ws
#define MDNS_INSTANCE "Mukklet"
#define MSG_MAX       4096 // longest text frame taken; a track message is well under 2 KB
#define CMD_MAX       96

static httpd_handle_t s_server;
static SemaphoreHandle_t s_lock; // guards the two below
static int s_fd = -1;            // socket of the current client
static player_t s_player;

// Cover state, apart from s_lock because the main loop holds it for a whole
// frame. The pixels are written without it (see cover.h).
static SemaphoreHandle_t s_cover_lock;
static cover_t s_cover;
static uint8_t s_cover_px[PROTO_COVER_BYTES];

static int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

void link_snapshot(player_t *out)
{
    if (!s_lock) { // not started yet
        player_init(out);
        return;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_player;
    xSemaphoreGive(s_lock);
}

const uint8_t *link_cover_acquire(const char *track_id)
{
    if (!s_cover_lock) return NULL;
    xSemaphoreTake(s_cover_lock, portMAX_DELAY);
    return cover_for(&s_cover, track_id);
}

void link_cover_release(void)
{
    if (s_cover_lock) xSemaphoreGive(s_cover_lock);
}

static void cover_locked(void (*fn)(cover_t *))
{
    xSemaphoreTake(s_cover_lock, portMAX_DELAY);
    fn(&s_cover);
    xSemaphoreGive(s_cover_lock);
}

// Sends a text frame to the current client; runs on the server task.
static void send_text(int fd, const char *text)
{
    httpd_ws_frame_t frame = {
        .final = true,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = (uint8_t *)text,
        .len = strlen(text),
    };
    esp_err_t err = httpd_ws_send_frame_async(s_server, fd, &frame);
    if (err != ESP_OK) ESP_LOGW(TAG, "send to fd %d failed: %s", fd, esp_err_to_name(err));
}

// httpd_queue_work() callback: sends a heap-allocated message to whoever
// is the client by the time it runs.
static void send_work(void *arg)
{
    char *text = arg;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    int fd = s_fd;
    xSemaphoreGive(s_lock);
    if (fd >= 0) send_text(fd, text);
    free(text);
}

bool link_send_cmd(cmd_t cmd, int arg)
{
    if (!s_server) return false;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool connected = s_fd >= 0;
    xSemaphoreGive(s_lock);
    if (!connected) return false;

    char *text = malloc(CMD_MAX);
    if (!text) return false;
    if (proto_cmd(text, CMD_MAX, cmd, arg) < 0) {
        free(text);
        return false;
    }
    ESP_LOGI(TAG, "-> %s", text); // before queueing: send_work frees it
    if (httpd_queue_work(s_server, send_work, text) != ESP_OK) {
        free(text);
        return false;
    }
    return true;
}

// Handshake done (ws_post_handshake_cb; this IDF doesn't call the handler
// for it): this client becomes the one, an older one is dropped.
static esp_err_t on_open(httpd_req_t *req)
{
    int fd = httpd_req_to_sockfd(req);
    xSemaphoreTake(s_lock, portMAX_DELAY);
    int old = s_fd;
    s_fd = fd;
    player_session_start(&s_player, now_ms());
    xSemaphoreGive(s_lock);
    cover_locked(cover_reset); // Mukk sends it again

    if (old >= 0 && old != fd) {
        ESP_LOGI(TAG, "new client (fd %d), closing the old one (fd %d)", fd, old);
        httpd_sess_trigger_close(s_server, old);
    } else {
        ESP_LOGI(TAG, "client connected (fd %d)", fd);
    }
    char hello[160];
    if (proto_hello(hello, sizeof(hello)) > 0) send_text(fd, hello);
    return ESP_OK;
}

// Reads and throws away the rest of a frame the display doesn't take.
static esp_err_t drain(httpd_req_t *req, httpd_ws_frame_t *frame)
{
    uint8_t scratch[256];
    frame->payload = scratch;
    esp_err_t err = ESP_OK;
    while (err == ESP_OK && frame->left_len > 0) err = httpd_ws_recv_frame_part(req, frame, sizeof(scratch));
    frame->payload = NULL;
    return err;
}

// A binary frame: the next piece of the cover, received straight into place.
static esp_err_t recv_cover_chunk(httpd_req_t *req, httpd_ws_frame_t *frame)
{
    uint8_t *dst = cover_chunk_dst(&s_cover, frame->len);
    if (!dst) {
        ESP_LOGW(TAG, "unexpected %u-byte cover frame, dropping the cover", (unsigned)frame->len);
        cover_cancel(&s_cover);
        return drain(req, frame);
    }
    frame->payload = dst;
    esp_err_t err = httpd_ws_recv_frame(req, frame, frame->len);
    frame->payload = NULL;
    if (err != ESP_OK) {
        cover_cancel(&s_cover);
        return err;
    }
    xSemaphoreTake(s_cover_lock, portMAX_DELAY);
    bool done = cover_chunk_done(&s_cover, frame->len);
    xSemaphoreGive(s_cover_lock);
    if (done) ESP_LOGI(TAG, "cover for %s", s_cover.track_id);
    return ESP_OK;
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    httpd_ws_frame_t frame = {0};
    esp_err_t err = httpd_ws_recv_frame(req, &frame, 0); // header only: type and length
    if (err != ESP_OK) return err;

    int fd = httpd_req_to_sockfd(req);
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool current = fd == s_fd;
    xSemaphoreGive(s_lock);

    // A stale client's last words are dropped.
    if (!current) return drain(req, &frame);
    if (frame.type == HTTPD_WS_TYPE_BINARY) return recv_cover_chunk(req, &frame);
    cover_cancel(&s_cover); // any other message ends a cover (never interleaved)
    if (frame.type != HTTPD_WS_TYPE_TEXT || frame.len > MSG_MAX) {
        if (frame.len > MSG_MAX) ESP_LOGW(TAG, "dropping a %u-byte frame", (unsigned)frame.len);
        return drain(req, &frame);
    }

    char *buf = malloc(frame.len + 1);
    if (!buf) return drain(req, &frame);
    frame.payload = (uint8_t *)buf;
    err = httpd_ws_recv_frame(req, &frame, frame.len);
    if (err == ESP_OK) {
        // Parsed outside the lock; proto_msg_t is ~800 bytes, too much for
        // the server task's stack next to cJSON.
        proto_msg_t *msg = malloc(sizeof(*msg));
        if (msg && proto_parse(buf, frame.len, msg)) {
            xSemaphoreTake(s_lock, portMAX_DELAY);
            player_apply(&s_player, msg, now_ms());
            xSemaphoreGive(s_lock);
            if (msg->type == MSG_TRACK) {
                ESP_LOGI(TAG, "track: %s - %s", msg->track.track.artist, msg->track.track.title);
            } else if (msg->type == MSG_COVER) {
                xSemaphoreTake(s_cover_lock, portMAX_DELAY);
                cover_begin(&s_cover, msg);
                xSemaphoreGive(s_cover_lock);
                if (!msg->cover.none && !s_cover.receiving) {
                    ESP_LOGW(TAG, "cover %dx%d, size %ld: not what hello asked for", msg->cover.w,
                             msg->cover.h, (long)msg->cover.size);
                }
            }
        } else if (msg) {
            ESP_LOGW(TAG, "not a protocol message: %.*s", (int)(frame.len > 80 ? 80 : frame.len), buf);
        }
        free(msg);
    }
    free(buf);
    return err;
}

// Every closed socket passes here (the server calls this instead of close()).
static void on_close(httpd_handle_t hd, int fd)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool current = fd == s_fd;
    if (current) {
        s_fd = -1;
        player_session_end(&s_player);
        ESP_LOGI(TAG, "client gone (fd %d)", fd);
    }
    xSemaphoreGive(s_lock);
    if (current) cover_locked(cover_reset);
    close(fd);
}

static esp_err_t start_mdns(void)
{
    esp_err_t err = mdns_init();
    if (err != ESP_OK) return err;
    mdns_hostname_set(MDNS_HOSTNAME);
    mdns_instance_name_set(MDNS_INSTANCE);
    return mdns_service_add(MDNS_INSTANCE, "_http", "_tcp", 80, NULL, 0);
}

esp_err_t link_start(void)
{
    s_lock = xSemaphoreCreateMutex();
    s_cover_lock = xSemaphoreCreateMutex();
    if (!s_lock || !s_cover_lock) return ESP_ERR_NO_MEM;
    player_init(&s_player);
    cover_init(&s_cover, s_cover_px);

    esp_err_t err = start_mdns();
    if (err != ESP_OK) ESP_LOGW(TAG, "mDNS not used (use the IP instead): %s", esp_err_to_name(err));

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true; // a vanished client doesn't hold a socket forever
    config.close_fn = on_close;
    config.stack_size = 6144; // cJSON parse of a track message
    err = httpd_start(&s_server, &config);
    if (err != ESP_OK) return err;

    const httpd_uri_t ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .is_websocket = true,
        .ws_post_handshake_cb = on_open,
    };
    httpd_register_uri_handler(s_server, &ws);
    ESP_LOGI(TAG, "serving ws://" MDNS_HOSTNAME ".local/ws");
    return ESP_OK;
}
