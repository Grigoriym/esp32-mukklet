# Mukklet: ESP32 now-playing display for Mukk

## Concept
A desk display showing what **Mukk** (own Linux music player, `../Mukk`,
Kotlin/Compose Desktop) is playing: cover art, title/artist, progress. The
KY-040 knob controls Mukk (play/pause, next/prev, volume). Music only ever
plays locally on the dev machine, in Mukk.

Sibling projects: `../esp32-desk-display` (weather/clock station, the
reference for patterns and code to lift: SSD1306 driver, encoder, WiFi,
mDNS + `esp_http_server`, host tests, CI, secrets) and `../esp32-hw-checks`
(bring-up tests for new modules; a new screen gets checked there first).

## Shared ESP32 docs (since 2026-10-02)
One source of truth per fact, never a copy (the user's rule, 2026-10-03):
- **Everything about a part** (pinout, voltage, current, measured
  dimensions, mounting, quirks, MACs and ports, test status, how many and
  which are free): its entry in the **Homebox inventory**
  (`http://192.168.0.139:34899`), nowhere else.
  `python3 ../grappim-watcher/docs/esp32/inventory/parts.py show <part>`
  prints an entry; plain `parts.py` lists what's free (`all`, `find
  <text>`). Read-only. Check it before suggesting a part or a purchase.
  When this project starts or stops using a part, that entry's `In use` /
  `Free` (in its description) has to change: the user does it in the UI,
  or a session through the API with the user's OK, never silently.
- Lessons that hold for any ESP32 project: `../grappim-watcher/docs/esp32/`
  (no git there):
- `WIRING_RULES.md`: which GPIOs are usable, power budget, cable
  conventions (one housing per cable, wires in the module's own order;
  when something already works, move the fewest pins).
- `ENCLOSURE_PLAYBOOK.md`: measuring, modelling, OpenSCAD and clash-check
  traps, hubs.com findings, explaining a case with labelled renders.
- `FIRMWARE_PLAYBOOK.md`: flash and serial-capture traps, config, code
  shape, the four checks.

**Rule: a new fact about a part goes in its Homebox entry, not here.** This
file keeps what is this project's own: pins, decisions, milestones, the
case design. When the printed case arrives, lessons that aren't about
this case go in `ENCLOSURE_PLAYBOOK.md`.

`../esp32-hw-checks` is a git repo since 2026-10-02 (local only, own
`CLAUDE.md`). Its knob and BLK pins are still the DevKit breadboard ones
(25/26/27, D4), not the mini's.

## Decisions so far (2026-09-27)
- **Link: direct WebSocket, ESP32 = server, Mukk = client.** No MPRIS, no
  bridge daemon, no cloud. Contract: `docs/PROTOCOL.md` (+ examples in
  `docs/protocol/`). MPRIS was considered: it would need a PC-side bridge to
  reach the ESP32 anyway (D-Bus is local); worth adding to Mukk later for
  media keys, independently of this.
- **Mukk side is built by a separate Claude Code session in `../Mukk`**
  from `docs/MUKK_TASK.md`. This repo owns the protocol; that session adds
  questions to the task file instead of changing the protocol.
- The display tells Mukk the cover size/format it wants (`hello`), and
  Mukk does all image work (crop, scale, dither, RGB565). The ESP32 never
  decodes JPEG/PNG. That's what makes the no-PSRAM board workable.
- **`tools/fake_display.py`** plays the ESP32's role on the PC (stdlib
  only): protocol checker + cover PNG dump + keyboard commands. Tested
  2026-09-27 against a scripted client (chunked mono1 cover, Cyrillic
  text, bad-field warnings).

## Hardware
What each part is (pinout, voltage, dimensions, quirks, MAC, port): its
Homebox entry, see "Shared ESP32 docs". Here: what this build uses and why.
- **Board: ESP32 mini #1** since
  2026-10-01. Same module as the DevKit, so the firmware didn't change. Why: on the EPLZON carrier the
  30-pin DevKit left only one free hole per pin row, no room to wire the
  modules. No carrier now (the EPLZON's joined rows would short the mini's
  inner and outer pins) and no mounting holes: held in a cradle (see
  Enclosure). Wired by the user 2026-10-02: pin headers pointing up (can
  side), joints underneath, on both inner rows and the right outer row;
  the TFT and knob cables plug on with female Dupont ends, the single 3V3
  pin feeds both modules through a Y-wire.
- **DevKit retired** (user, 2026-10-01:
  "forget about the old board"). Its breadboard wiring still has the knob
  on 25/26/27 and BLK on D4. It and the mini both answer as
  `mukklet.local`: power only one at a time.
- **Screen: 1.69" 240×280 ST7789V2**, which of the two is in the build doesn't
  matter (user, 2026-10-02): don't ask. **Chosen by the user (2026-09-27)** over the Waveshare 2.0"
  240×320 ST7789V IPS (same wiring, labels DIN/CLK/RST/BL; the fallback
  if 1.69" turns out too small). Rejected: 1.8" 128×160 ST7735 (low-res,
  often TN), 1.3" 240×240 (tiny, many have no CS pin), 2.8" ILI9341 (TN,
  resistive touch).
- **Cover size**: the user prefers **more text over a big cover**
  (2026-09-27): ask Mukk for 160×160 `rgb565` (51 KB, fits in RAM, so it
  can be kept and redrawn), text below it (title, artist, album (year),
  progress; no room for "Next:" at 280 px).
- **Pins** (tables, housings, cable lengths, the rejected in-order
  layout: `docs/WIRING.md`): TFT SCL D18, SDA D23, RES GPIO17, DC GPIO16,
  CS D5, BLK D19 (was D4 until 2026-10-01); knob CLK D27 / DT D25 / SW D32
  (was 25/26/27 until 2026-10-01). Picked so the cables end in few
  housings on the mini: 18/19/23/5/3V3 are one 5-pin on the left inner
  row, 17/16/GND one 3-pin on the right inner row, GND/27/25/32 one 4-pin
  on the right outer row in the KY-040's own CLK, DT, SW order (the
  user's choice). Only BLK moved for the TFT. I2C D21/D22 stay free, D2
  (onboard LED) is avoided.
- **Status**: mini flashed 2026-10-01 (boots, joins WiFi, Mukk connected
  and sent a track + cover); TFT wired to it the same day, picture fine
  per the user; knob on the new pins and everything connected 2026-10-02,
  "seems to work fine" per the user.
- **OLED**: milestone 1's prototype screen
  (asked Mukk for 64×64 `mono1`), to find out which info is worth
  showing. No I2C code since 2026-09-28.

## Firmware (milestone 3: colour screen + cover art, 2026-09-28)
WiFi + mDNS `mukklet.local` + WebSocket server (`link.c`) + TFT text,
cover art + knob commands. Milestone 2 (text only) and milestone 1 (OLED, 2026-09-27) is in git history before
this; the OLED code is gone. Pure, host-tested modules (no ESP-IDF):
`proto` (parse/build messages), `player` (session, position extrapolation,
15 s offline rule), `ui` (the screen from player state + time, stateless
incl. scrolling), `canvas` (RGB565 band of the screen), `font` (UTF-8,
anti-aliased), `gesture` (single/double/long press), `encoder_decode`
(lifted).
- **Drawing in strips**: `display_frame()` renders the whole UI once per
  band of 20 rows into one 9.6 KB DMA buffer (`ui_render` skips what's
  outside the band), hashes each strip and only sends the ones that
  changed. A test checks the strips add up to the full-height render.
- **Display driver**: `esp_lcd` ST7789, 40 MHz, RGB order + INVON, gap
  (0, 20), backlight via LEDC on BLK (dark until the first frame).
  `FLIP_180` in `display.c`: 1 = upright with the pin header at the
  bottom; final mounting not decided (user: "fine for now").
- **Font**: DejaVu Sans (Bitstream Vera license), 4-bit anti-aliased,
  proportional: `FONT_TITLE` bold 20 px, `FONT_TEXT` 16 px. ASCII,
  Latin-1, Latin Ext-A, Romanian, Cyrillic, typographic punctuation.
  Generated into `main/font_data.c` by `tools/gen_font.py` (needs Pillow
  + fonts-dejavu-core; rerun, don't hand-edit), ~69 KB. Unknown characters
  draw as a box. Size readable on the panel, per the user.
- **Cover art** (milestone 3, 2026-09-28): `hello` asks for 160x160
  `rgb565` (`PROTO_COVER_*` in `proto.h`); pure `cover.c` assembles the
  binary frames straight into a static 51 KB buffer in `link.c` (own
  mutex, held by `main.c` for a whole frame via
  `link_cover_acquire/release`); `canvas_image_be()` + `ui.c` draw it in
  place of the grey placeholder. DRAM 87 KB free with it.
- **Layout** (240x280): 160x160 cover (grey placeholder until one arrives) at the top, title /
  artist / album (year), each centred or scrolling within 12 px margins,
  progress bar, then status icon, position, volume %, duration (18 px
  margins for the rounded corners).
- **Idle screens** (2026-09-28, "seems nice" per the user): bored
  cartoon eyes above "Waiting for Mukk" and "Nothing playing" (`eyes()`
  in `ui.c`): look around, blink, roll, droop, nod off, sleep with z's,
  wake with a start; a ~2 min cycle from `screen_since_ms` (`main.c`
  resets it whenever `ui_screen()` changes). Anti-aliased
  `canvas_ellipse()`. Tuning constants at the top of that block.
- **Knob**: turn = volume ±5 per detent (a fast spin is one `cmd`), press =
  play/pause (sent 300 ms after release, waiting for a double), double =
  next, long (0.6-2 s, sent on release) = prev, hold 2 s = screen off
  (backlight 0, no drawing; chosen by the user 2026-09-28 over triple
  press / auto-off). While off, any input only wakes it. Verified on the
  board by the user (2026-09-28).
- **Window test screens**: `EDGE_TEST` in `main.c` (1 = coloured 2 px frames
  from each edge, 2 = quarter circles 10..45 px in the bottom corners) replaces
  the UI, for checking a printed window against the lit area. Keep it 0 in git.
  On this panel the lit area's bottom corners are rounded (~42 px), the top ones
  not.
- IDF 6 gotcha: the WS handler is **not** called for the handshake any
  more; `hello` goes out from `ws_post_handshake_cb`
  (`CONFIG_HTTPD_WS_POST_HANDSHAKE_CB_SUPPORT`).
- Verified on the board 2026-09-28 with the **real Mukk**: connects,
  shows tracks, right way up (pins at the bottom), text readable, per the
  user. Cover art verified the same way (same day): arrives about 1 s after
  connect, looks right per the user.

## Open questions
- Which fields to show: the colour layout is a first guess.
- Mounting orientation (see `FLIP_180`): the enclosure draft assumes pins at the bottom.

## Build / flash
ESP-IDF, same setup as the desk display:
`. ~/esp/esp-idf/export.sh && idf.py -p /dev/ttyACM0 build flash`
(the mini; the weather station is a `ttyUSB` board).
Flashing and checking a build on the board: use the `esp32-flash-verify`
skill (generic routine); this section keeps only what's specific to this
repo. Other flash and serial
traps (missing port, identity check by MAC, serial noise, no TTY for
`idf.py monitor`, checks that need hands, `pkill -f`):
`FIRMWARE_PLAYBOOK.md` in the shared docs.
Mukk connects to the real board (`mukklet.local`) by default: to test
against `tools/fake_display.py`, point Mukk's display host at
`localhost:8765` first, or the fake never sees a connection.
Boot logs: `tools/serial_log.py <seconds> "<regex>"` with the IDF python
env (port `/dev/ttyACM0`, or `PORT=...` in the environment; it resets the
board, so the screen restarts; if the board is unplugged mid-capture it
prints what arrived and exits non-zero).

Checks (same as CI, `.github/workflows/ci.yml`; IDF env sourced):
`tools/test.sh` (host unit tests; `SHOW_ART=1` prints the rendered screens
as ASCII art), `tools/format.sh --check`, `tools/lint.sh`,
`tools/size_check.sh`. WiFi credentials: `main/wifi_secrets.h` (gitignored,
template `.example`).

Playing Mukk's role: `python3 tools/fake_mukk.py [--host IP] [--seconds N]`
connects to the display, sends a small playlist (Cyrillic, long title, no
tags, unknown duration) and obeys the knob, printing each `cmd`.

## Enclosure (since 2026-09-29)
OpenSCAD in `enclosure/` (see its README), lifted from the desk
display's `enclosure_v2.scad` ("cut" carrier) and its lessons (check fits
on the perfboard's hole grid; sink solids into walls, not tangent). The
user left the design to me ("just create something, we will work it
out"): first draft is a 50 x 82 x 62 wedge, TFT portrait on a panel tilted
20°, pins at the bottom (so `FLIP_180` stays 1), knob on top centred, USB
out the back. **ESP32 mini cradle (2026-10-01)**: the mini lies flat on
the base at the back, can up, 3 mm above the floor, no screws and nothing
that flexes: antenna end under two lips at the front corners, back end on
two rests with a stop behind, side guides, and a ledge on the body's back
wall 0.2 above the USB socket (holds the back down once the base is
screwed on). Why the ledge sits on the socket: it's the one measured
height at the back; the board's back corners have LEDs/parts close to the
edge (photo only). Pin headers up, Dupont plugs on top (tops 24 mm above
the floor, knob board at 48; TFT plug ends ~12 mm in front). Outside still
50 x 82 x 62 for the cardboard mock-up, but its USB hole is now 16 mm
lower (z 9, was 25) and 0.25 left of centre seen from the front. The
EPLZON carrier and the DevKit are out of the model (git history).
**Three printed parts since 2026-10-02** (was a one-piece shell + base):
`body` (side walls, top, back), `front` (the tilted screen panel, a flat
plate between the side walls on two rails, 4 x 2.3 x 8 screws from the
front, heads visible: fine by the user) and `base` (floor + the 12 mm
strip under the screen + the mini's cradle). Why: in the one-piece shell
no screwdriver could reach the TFT's two top screws (their line hit the
back wall; found only by drawing a driver along each screw axis, the
clash check can't see it). The user asked for "modular" themselves.
`test_front` is gone (the plate is the test); `test_knob` is the top's
front with the knob mount. Printed 2026-10-07 (v1, see Next step); the
desk display's case is still unprinted.
Before ordering a print: the `enclosure-preprint-review` skill; the model's
"pre-print checks" asserts (plastic round holes, pilot depth vs screw reach,
gaps) run in `export.sh`, which also lists every ASSUMED value.
`enclosure/export.sh` = clash check (printed parts vs stand-ins, printed
parts vs each other, and
stand-ins incl. plugs vs each other, and fails on undefined-variable
warnings) + STLs + renders.
Part dimensions: each part's Homebox entry (`enclosure/MEASUREMENTS.md`
says which parts and what the model does with them). Generic lessons from this case (measuring, clash check, tool
access, screw lengths, hubs.com, OpenSCAD traps): `ENCLOSURE_PLAYBOOK.md`
in the shared docs. This case's own notes:
- hubs.com (the user uploads there): the three parts pass with a 1.2 mm
  straight edge before the window's chamfer and 5.5 mm posts; its 95 EUR
  quote was too expensive for the user (2026-10-02).
- "Mukklet Assembly", `enclosure/pages/assembly/assembly.html`: labelled
  renders, redone for three parts 2026-10-02. Pages like it are local
  HTML files in `enclosure/pages/`, not claude.ai artifacts.
- The mini's cradle works as printed (2026-10-07): the lips came with
  support to cut out, the tilt-in move and the ledge over the socket work.
- The TFT's plug hanging down behind the screen is what limits a
  carrier: at 82 deep the EPLZON's front passed over it only at >= 8 mm
  standoffs, and its front middle M3 hole sat right over the plug.

## Next step
Milestone 3 (cover art) done. Enclosure **v1 printed (local FDM shop) and
assembled 2026-10-07**: cradle, USB, front plate and TFT work; the knob
mount doesn't hold the board (the user balances it when pressing). Findings
in `enclosure/archive/v1/README.md` (v1 files archived there); v2 in
`enclosure/README.md`. In the model
since 2026-10-10: the base covers the whole footprint (walls stand on it;
v1's base holes were 0.2 from its edge), base holes 4.0, deeper boss
pilots; front screws 2 x 6 (two 2.3 x 8 snapped in v1); the knob held by
its own M7 nut through a 7.3 hole (+ two anti-turn ribs) instead of snap
hooks (KY-040 measured, in Homebox; `enclosure/pages/knob/knob.html`).
Open, in order:
1. **Bug fixing** (firmware/Mukk side): next, when the user says so
   (2026-10-07: "after, not now, i will notify"). No list yet.
2. **Case v2**: sent to the local FDM shop 2026-10-10 (three STLs, PETG
   asked for). When it's back: the printed-part checklists in
   `enclosure/README.md` (knob hole 7.3, front pilots, base fully in,
   EDGE_TEST 1 and 2 for the window).
3. Which way clockwise turns the volume was never said.
After a model change, `enclosure/pages/assembly/render.sh` redoes the
assembly page's pictures and `~/Videos/mukklet-reel/renders/render.sh` the
reel's clips.
