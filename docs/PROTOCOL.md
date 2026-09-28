# Mukk ↔ Mukklet protocol (v1)

The contract between **Mukk** (desktop player, `../Mukk`) and the **ESP32
now-playing display, Mukklet** (this repo). Both sides implement exactly this; a
change here is a change on both sides.

## Transport

- **WebSocket.** The **ESP32 is the server**, **Mukk is the client**.
- URL: `ws://mukklet.local/ws` (port 80). The ESP32 announces
  `mukklet.local` over mDNS; on this Linux machine `.local` names resolve
  through the system resolver (`nss-mdns`), so a plain JVM
  `InetAddress`/WebSocket client resolves it without any mDNS library.
- One client at a time. If a second client connects, the ESP32 keeps the
  newest one and closes the old one.
- LAN only, no auth, no TLS.

Why this direction: the display is the thing with a fixed name on the
network (same pattern as `desk.local` in `../esp32-desk-display`), and a
display that reboots or is unplugged just means Mukk retries quietly.

## Framing

- **Text frames**: one JSON object each, UTF-8, always with a `"type"` field.
- **Binary frames**: only for cover-art pixel data, and only right after a
  `cover` text message (see below).
- **Unknown `type` values and unknown fields must be ignored** by both
  sides, not treated as errors. That is how the protocol grows without a
  version bump.
- Strings are sent as they are in the tags (full UTF-8, including Cyrillic
  etc.). Mukk does **not** transliterate or truncate; fitting text to the
  screen is the display's job.

## Session flow

```
Mukk                                   ESP32
 |---- WebSocket connect ------------->|
 |<--------------------- hello --------|   what the display can show
 |---- track ------------------------->|   current track (or none)
 |---- cover + binary chunks --------->|   art in the format hello asked for
 |---- state ------------------------->|
 |          ... then, as things happen ...
 |---- track / cover / state --------->|
 |<----------------------- cmd --------|   knob: play/pause, next, volume...
 |---- state ------------------------->|   the result of the command
```

## Messages: ESP32 → Mukk

### `hello`
Sent once, immediately after the connection opens. Mukk must wait for it
before sending anything (it needs `cover` to prepare the art).

```json
{
  "type": "hello",
  "v": 1,
  "device": "mukklet-oled",
  "cover": { "w": 64, "h": 64, "format": "mono1" },
  "maxChunk": 4096
}
```

| Field | Meaning |
|---|---|
| `v` | Protocol version, `1`. |
| `device` | Free-form name, for Mukk's log only. |
| `cover.w`, `cover.h` | Exact pixel size the display wants the cover in. |
| `cover.format` | `"mono1"`, `"rgb565"` or `"none"` (see Cover formats). `"none"` = don't send art. |
| `maxChunk` | Max bytes per binary frame. Mukk splits the pixel data into frames of at most this size. |

### `cmd`
A control input on the display (the knob). Mukk applies it exactly as if
the user did the same thing in Mukk's own UI, then sends a fresh `state`
(and `track`/`cover` if the track changed).

```json
{ "type": "cmd", "cmd": "play_pause" }
{ "type": "cmd", "cmd": "next" }
{ "type": "cmd", "cmd": "prev" }
{ "type": "cmd", "cmd": "volume", "delta": 5 }
{ "type": "cmd", "cmd": "seek", "deltaMs": 10000 }
```

| `cmd` | Behaviour in Mukk |
|---|---|
| `play_pause` | Toggle. If idle/stopped with a track selected, start playing it. |
| `next` / `prev` | Same as the transport bar buttons (respects repeat/shuffle). |
| `volume` | `delta` in percentage points (may be negative); clamp to 0–100. |
| `seek` | `deltaMs` relative to the current position (may be negative); clamp to 0–duration. |

Unknown `cmd` → ignore (log at debug).

## Messages: Mukk → ESP32

### `track`
Sent after `hello`, and every time the current track changes. When nothing
is loaded, send `"track": null`.

```json
{
  "type": "track",
  "track": {
    "id": "a3f9c2",
    "title": "Paranoid Android",
    "artist": "Radiohead",
    "album": "OK Computer",
    "albumArtist": "Radiohead",
    "year": 1997,
    "genre": "Alternative",
    "trackNo": 2,
    "durationMs": 386000,
    "format": "FLAC",
    "hasCover": true
  },
  "next": { "title": "Subterranean Homesick Alien", "artist": "Radiohead" }
}
```

| Field | Notes |
|---|---|
| `id` | Any string that is **unique per track** (e.g. a short hash of the file path). The display uses it to match a `cover` to its track. |
| `title` | If the tag is empty, the **file name without extension**. Never empty. |
| `artist`, `album`, `albumArtist`, `genre` | `""` when missing. |
| `year`, `trackNo` | `0` when missing. |
| `durationMs` | `0` if unknown. |
| `format` | File extension upper-cased (`MP3`, `FLAC`, ...). |
| `hasCover` | Whether a `cover` with pixels will follow. |
| `next` | The track `next` would play, or `null` when it isn't known (shuffle on, end of folder with repeat off). |

### `cover`
Sent right after every `track` (unless `hello` said `"format": "none"`).

With art:
```json
{ "type": "cover", "trackId": "a3f9c2", "w": 64, "h": 64, "format": "mono1", "size": 512 }
```
followed by **binary frames** whose payloads, concatenated in order, are
exactly `size` bytes of pixel data. Each frame ≤ `maxChunk` bytes. No other
message is sent in between.

Without art (tag has none, or it failed to decode):
```json
{ "type": "cover", "trackId": "a3f9c2", "none": true }
```

If a newer `track` arrives before the display has all the chunks of an
older cover, the display drops the partial one. Mukk never interleaves two
covers.

### `state`
Sent after `hello`, on every status/volume/seek change, and **every 5
seconds regardless** (it doubles as the heartbeat: no message for 15 s
means the display shows "Mukk offline").

```json
{
  "type": "state",
  "status": "playing",
  "positionMs": 125340,
  "volume": 80,
  "repeat": "all",
  "shuffle": false
}
```

| Field | Values |
|---|---|
| `status` | `"playing"`, `"paused"`, `"stopped"`, `"idle"` (Mukk's `PlaybackStatus`, lower-cased). |
| `positionMs` | Current position. The display extrapolates between messages while `playing`. |
| `volume` | 0–100 (Mukk's 0.0–1.0 × 100, rounded). |
| `repeat` | `"off"`, `"one"`, `"all"`. |
| `shuffle` | boolean. |

## Cover formats

Mukk decodes the embedded art (the same bytes the Now Playing panel shows),
**center-crops it to the target aspect ratio**, scales it to exactly
`w × h` with a good-quality filter, then encodes:

| `format` | Bytes | Encoding |
|---|---|---|
| `mono1` | `w*h/8` | 1 bit per pixel, rows top to bottom, each row left to right, **MSB = leftmost pixel**, `1` = lit (white). Convert to grayscale, then **Floyd–Steinberg dither** at threshold 128. `w` is a multiple of 8. |
| `rgb565` | `w*h*2` | 16 bits per pixel, rows top to bottom, **big-endian** (high byte first: the order ST7789 panels take over SPI). |

The colour TFT (240×280) asks for `rgb565` 160×160 (51 200 bytes, 13
frames of 4096), with text below the cover; the OLED prototype asked for
`mono1` 64×64. Mukk supports both, so switching screens needs no Mukk
change.

## Reconnects and robustness (Mukk side)

- Feature off by default; a setting enables it and holds the host
  (default `mukklet.local`). The host may carry a port
  (`localhost:8765` for `tools/fake_display.py`); without one it's 80.
- Connect in the background; on failure or disconnect, retry with backoff
  (2 s, doubling, capped at 30 s). Never block playback or the UI on the
  display, and never show an error dialog for it (log at debug/warn).
- After every (re)connect, the full sequence again: wait for `hello`, then
  `track`, `cover`, `state`.

## Examples

Complete example messages are in `docs/protocol/`.
`tools/fake_display.py` plays the ESP32's role on the PC for testing Mukk
without hardware.
