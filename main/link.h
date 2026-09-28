#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "player.h"

// The link to Mukk (docs/PROTOCOL.md): announces mukklet.local over mDNS
// and serves the WebSocket at ws://mukklet.local/ws. One client at a time,
// the newest wins. Messages from Mukk update a player_t that the main loop
// reads with link_snapshot().

// Starts mDNS and the server. Call once WiFi is up (or trying to be).
esp_err_t link_start(void);

// Copies the current player state.
void link_snapshot(player_t *out);

// The finished cover art of this track (PROTO_COVER_BYTES, big-endian), or
// NULL. Holds the cover until link_cover_release(), so a new one waits
// instead of overwriting it mid-frame: keep it for one frame only.
const uint8_t *link_cover_acquire(const char *track_id);
void link_cover_release(void);

// Sends a "cmd" to the connected client. False if there's none (or the
// send couldn't be queued).
bool link_send_cmd(cmd_t cmd, int arg);
