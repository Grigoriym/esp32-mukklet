# Task for the Mukk session: Mukklet display link

**Hand this file to a Claude Code session in `../Mukk`.** The ESP32 side
lives in this repo and is built separately; Mukk only needs to speak the
protocol.

## Goal

Mukk streams what it is playing (track info, cover art, playback state) to
a small ESP32 display on the LAN, and obeys play/pause/next/prev/volume/
seek commands coming back from the display's knob.

**The contract is `/home/gregory/proj/grappim/esp32-mukklet/docs/PROTOCOL.md`
(read it in full first).** Example messages:
`/home/gregory/proj/grappim/esp32-mukklet/docs/protocol/*.json`. Don't extend or change
the protocol from the Mukk side; if something in it doesn't fit Mukk,
note it in the "Open questions" section at the bottom of this file and
leave it for the ESP32 session.

## What to build

1. **A `DisplayLink` component** (new; a new `core:*` module or inside
   `composeApp`, your call; follow Mukk's module-boundary rules in its
   `CLAUDE.md`). WebSocket **client**. Suggested: the JDK's built-in
   `java.net.http.HttpClient.newWebSocketBuilder()`, which needs no new
   dependency. It:
   - connects to `ws://<host>/ws`, waits for `hello`, then sends
     `track` → `cover` → `state`;
   - observes the player (`AudioPlayer.state` / the ViewModel's flows)
     and sends `track`+`cover` on track change, `state` on
     status/volume/seek/repeat/shuffle change **and every 5 s** (heartbeat);
   - reconnects with backoff (2 s doubling to 30 s), never blocks
     playback or the UI, never shows an error dialog;
   - closes cleanly on app exit (`main.kt` already disposes `AudioPlayer`
     there).
2. **Cover encoding** (pure functions, unit-tested): raw embedded art
   bytes (`MetadataReader.readAlbumArt()`) → decode → center-crop to the
   target aspect → scale to `w×h` → `mono1` (grayscale + Floyd–Steinberg
   dither, 1 bpp MSB-first, 1 = white) or `rgb565` (big-endian). Then split
   into binary frames of ≤ `maxChunk` bytes. Plain `java.awt.image` is
   enough.
3. **Commands from the display** → the same ViewModel/actions the UI uses
   (`togglePlayPause()`, `nextTrack()`, `previousTrack()`, `setVolume()`,
   `seekTo()` in `MukkViewModel`), so behaviour matches the transport
   bar exactly. Send a fresh `state` after applying one.
4. **Settings** (Mukk's usual path: `SettingsState` → ViewModel →
   `PreferencesManager` → `SettingsDialog`): "Mukklet display"
   on/off (default **off**) and host (default `mukklet.local`, may
   include `:port`). Optional: a small connected/disconnected indicator
   in the dialog.

## Field mapping hints

| Protocol | Mukk source |
|---|---|
| `track.id` | short stable hash of the file path |
| `title` / `artist` / `album` / `albumArtist` / `genre` / `year` / `trackNo` | `MediaTrackData` (DB) or `AudioMetadata`; title falls back to file name without extension |
| `durationMs` | `PlaybackState.durationMs` / `MediaTrackData.duration` |
| `format` | file extension, upper-cased |
| `next` | what `nextTrack()` would pick; `null` when shuffle makes it unknown or nothing follows |
| `status` | `PlaybackState.playbackStatus`, lower-cased |
| `volume` | `round(PlaybackState.volume * 100)` |
| `repeat` / `shuffle` | `SettingsState.repeatMode` / `shuffleEnabled` |

## How to test without the hardware

From this repo (the path is `../esp32-mukklet` relative to Mukk):

```
python3 tools/fake_display.py                          # acts like the OLED: 64x64 mono1
python3 tools/fake_display.py --format rgb565 --size 160   # acts like the TFT
```

Standard library only. Set Mukk's display host to `localhost:8765`. The
script prints every message, **flags protocol violations with
`!! PROTOCOL:`**, draws mono covers as text art, saves every cover as a
PNG under `/tmp/fake_display/`, and sends commands when you type `p`, `n`,
`b`, `+`, `-`, `f`, `r` + Enter.

## Done when

- [ ] With the fake display running, playing a track shows `track`, a
      `cover` whose PNG looks right (both `mono1` 64 and `rgb565` 240),
      and `state` every 5 s, with **no `!! PROTOCOL` lines**.
- [ ] Track change, pause, seek, volume change in Mukk's UI each produce
      the right message within ~1 s.
- [ ] Every fake-display key does the same as the matching Mukk button.
- [ ] Killing and restarting the fake display: Mukk reconnects on its
      own, no UI freeze, no error dialog. With the feature off, Mukk
      makes no connection attempts.
- [ ] A track with no embedded art sends `cover` with `"none": true`.
- [ ] Unit tests for the cover encoder (crop/scale/dither/rgb565 byte
      order, chunking) pass; detekt passes.
- [ ] Mukk's `CLAUDE.md` documents the new component and setting.

## Open questions (Mukk session: add here, don't change the protocol)

- **`play_pause` from idle/stopped** (2026-09-27): Mukk only replays its
  last current track; with no current track it does nothing (PROTOCOL.md
  says "start the selected track"). Mukk keeps its transport-bar behaviour.
  *ESP32 side: accepted as is; the display just shows the resulting
  `state`.*
- **`next` under repeat ONE** (2026-09-27): Mukk sends the current track as
  `next`, since that is what it would play. *ESP32 side: accepted, matches
  the field's meaning ("the track `next` would play").*
- FYI: `next` follows Mukk's *selected* folder in the tree, not always the
  playing folder (existing Mukk behaviour).
