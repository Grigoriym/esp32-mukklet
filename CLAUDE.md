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
- **Board**: same ESP32-WROOM-32 DevKit as the desk display (ELEGOO, 30-pin,
  CP2102, USB-C, 4 MB flash, **no PSRAM**, ~320 KB RAM). Spares from the
  3-pack, so the weather station stays untouched.
- **Prototype screen**: the 128×64 SSD1315 I2C OLED (spare from the desk
  display's 3-pack) on D21/D22. Purpose: find out which info is actually
  worth showing before buying the colour screen. Asks Mukk for 64×64 `mono1`.
- **Target screen (not bought yet)**: 1.69" IPS 240×280 **ST7789V2** 4-wire SPI
  (GERUI, pack of 2): 3.3 V only (VCC → 3V3, never VIN), 8 pins
  GND/VCC/SCL/SDA/RES/DC/CS/BLK (SCL/SDA = SPI clock/data), BLK high = on
  (PWM for dimming), rounded corners, needs a **20 px row offset**
  (controller is 240×320), active area 27.97×32.63 mm, PCB 31×48 mm, 4× M2 holes 26×43 mm.
  Full frame 134 KB: draw in strips, never hold a full framebuffer. Cover size: the
  user prefers **more text over a big cover** (2026-09-27): ask Mukk for
  ~160×160 `rgb565` (51 KB, fits in RAM, so it can be kept and redrawn),
  text below it (title, artist, album (year), progress; no room for
  "Next:" at 280 px). **Chosen by the user (2026-09-27)** over the
  Waveshare 2.0" 240×320 ST7789V IPS (same wiring, labels DIN/CLK/RST/BL;
  the fallback if 1.69" turns out too small) and a 2.8" ILI9341 (TN).
  Rejected: 1.8" 128×160 ST7735 (low-res, often TN), 1.3" 240×240 (tiny,
  many have no CS pin), 2.8" ILI9341 (TN, resistive touch).
- **Both ST7789V2 screens arrived and checked (2026-09-28)** with
  `../esp32-hw-checks` (ST7789 test via `esp_lcd`, 40 MHz SPI on the
  breadboard, wiring as planned below, one test per knob press): colours
  right with RGB order + inversion on, 20 px row offset right (border
  visible on all 4 edges), smooth grey ramp, BLK PWM fade smooth. Both
  show the picture **rotated 180° with the pin header at the bottom**
  (not mirrored): fix in software for however it gets mounted. Screen #1
  has one small dark dot (fine by the user); #2 is clean.
- **OLED no longer used** (2026-09-28): the firmware has no I2C code, and
  the user was told the OLED can be unplugged from D21/D22 (not confirmed
  whether they did).
- **TFT wiring** (all pins: `docs/WIRING.md`): SCL D18, SDA D23, RES GPIO17 (TX2), DC GPIO16
  (RX2), CS D5, BLK D19 (was D4 until 2026-10-01; on the mini 18/19/23/5/3V3 are 5 pads in a row on the left inner row = one 5-pin housing, and 17/16/GND one 3-pin on the right inner row). Keeps I2C D21/D22 and the knob D27/D25/D32 free,
  avoids D2 (onboard LED).
- **Knob**: KY-040 on D27 (CLK) / D25 (DT) / D32 (SW) (was 25/26/27 until 2026-10-01: moved so GND/27/25/32 are 4 pads in a row on the mini's right outer row, one 4-pin housing, in the KY-040's own CLK, DT, SW order: the user's choice), as on the desk
  display; lift `encoder.c` + `encoder_decode.c` from there.
- **Prototype wired and checked (2026-09-27)** on a breadboard, every module
  straight to the ESP32 (no daisy-chaining), both on 3V3: OLED VCC/GND/SDA
  D21/SCL D22, KY-040 +/GND/CLK D25/DT D26/SW D27. Flashed
  `../esp32-hw-checks`: I2C scan finds only **0x3C**, panel shows HELLO
  (upright or not: not recorded; set orientation in software, as on the
  desk display), knob 4 steps/detent both ways, rest state CLK=1 DT=1,
  button clean (5 presses, 5 releases). First clockwise turn logged `CW`;
  that it matches the physical direction is **not yet confirmed** by the
  user. The board still runs hw-checks.
- **Final board: ESP32 "D1 mini" style (ordered 2026-09-30, arrived and
  measured 2026-10-01: `enclosure/MEASUREMENTS.md`, 31.51 x 39.02, no
  mounting holes, pins not soldered yet)**, pack of 3, CH9102F USB-C (MH-ET LIVE MiniKit layout, ~39 x 31
  mm, 2 x 10 pads per side). Why: on the EPLZON carrier the 30-pin DevKit
  left only one free hole per pin row, no room to wire the modules. Same
  WROOM-32 module (same RAM, no PSRAM), so the firmware and every pin stay
  as they are (table in `docs/WIRING.md`); no carrier (the EPLZON's joined rows would short its inner and outer
  pins). Agreed 2026-10-01 (the model is built on it), not soldered yet: pin headers
  pointing up (can side), joints underneath, on both inner rows and the
  right outer row; TFT and knob wires plug on with female Dupont ends
  (the user has a crimp tool and a Dupont kit with 1-6 pin housings;
  needs 26 AWG stranded wire, their 22 AWG is solid); the single
  3V3 pin feeds both modules through a Y-wire (two wires crimped into one
  terminal). Housings at the mini: 5-pin + 3-pin for the TFT, 4-pin for
  the knob, cables 12 / 15 cm (table and the rejected in-order layout:
  `docs/WIRING.md`). The user wants pins chosen so a cable ends in one
  housing with the wires in the module's own order; when something
  already works, move the fewest pins (only BLK moved for the TFT). Seller photos show no
  mounting holes: held in a cradle (see Enclosure). The DevKit is retired (user, 2026-10-01: "forget about the old board"); its breadboard wiring still has DT on D26.
- **Mini #1 plugged in and flashed (2026-10-01)**, first bare (nothing wired to
  it): `/dev/ttyACM0` (CH9102F is CDC-ACM; `PORT=/dev/ttyACM0` for
  `tools/serial_log.py`), MAC `20:50:0d:2a:0a:7c`, ESP32-D0WD-V3 rev 3.1,
  40 MHz crystal, 4 MB flash. Flashed at the default baud without trouble;
  boots, joins WiFi, Mukk connected and sent a track + cover. It and the
  DevKit both answer as `mukklet.local`: power only one at a time. Same day: TFT wired to it (BLK on IO19), picture fine per the user; knob on the new pins (27/25/32) not confirmed yet.
- **DevKit: MAC `70:4b:ca:4d:f0:1c`** (ESP32-D0WD-V3 rev 3.1, 40 MHz
  crystal), retired, was on `/dev/ttyUSB0`. Read-only identity check before flashing:
  `esptool -p /dev/ttyACM0 chip-id`. The weather station's board is a
  different one; if it's online, `desk.local` answers over WiFi.
- When it arrived here it ran unknown firmware that printed unreadable
  bytes at **every** baud rate (57600-921600, incl. 115200/74880), yet
  esptool connected fine: the garbage was that firmware, not the wiring or
  the USB link. Flashing replaced it. If a board's serial log looks like
  noise, try `esptool chip-id` before suspecting the wiring.

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
(the mini; the weather station is a `ttyUSB` board). Check
`ls /dev/tty{USB,ACM}*` and the MAC above before flashing. The user
unplugs the mini to work on the wiring: if the port is gone, the flash
fails with "port is busy or doesn't exist" after a good build; check the
port first and ask for it to be plugged in.
If flashing fails with "Serial data stream stopped: Possible serial noise"
(happened 2026-09-28 at the default and 460800 baud), add `-b 115200`.
Mukk connects to the real board (`mukklet.local`) by default: to test
against `tools/fake_display.py`, point Mukk's display host at
`localhost:8765` first, or the fake never sees a connection.
Boot logs (`idf.py monitor` needs a TTY the harness doesn't have):
`tools/serial_log.py` resets the board and captures, run with the IDF
python env,
`~/.espressif/python_env/idf6.2_py3.14_env/bin/python tools/serial_log.py <seconds> "<regex>"`
(port `/dev/ttyACM0`, or `PORT=...` in the environment; it resets the
board, so the screen restarts; if the board is unplugged mid-capture it
prints what arrived and exits non-zero).
Checks that need hands (turning the knob): run the capture with
`run_in_background`, tell the user what to do, read the output when it ends.

Checks (same as CI, `.github/workflows/ci.yml`; IDF env sourced):
`tools/test.sh` (host unit tests; `SHOW_ART=1` prints the rendered screens
as ASCII art), `tools/format.sh --check`, `tools/lint.sh`,
`tools/size_check.sh`. Format and lint only see **git-tracked** files:
`git add` new ones first, or CI catches what the local run missed. WiFi credentials: `main/wifi_secrets.h` (gitignored,
template `.example`).

To stop a background `fake_display.py`, kill its task (or use `pgrep` + `kill`
on the PID): `pkill -f fake_display.py` matches the shell running that
command too, and kills it (exit 144).

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
two rests with a stop behind, side guides, and a ledge on the shell's back
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
front with the knob mount. Nothing has been printed yet, the desk
display's case neither, so every fit is untested.
`enclosure/export.sh` = clash check (printed parts vs stand-ins, printed
parts vs each other, and
stand-ins incl. plugs vs each other, and fails on undefined-variable
warnings) + STLs + renders. TFT measured 2026-09-29 (`MEASUREMENTS.md`,
"Mukklet Caliper Guide", `enclosure/pages/caliper.html`): ears + a notch at the
top with the glass's flat cable wrapping round it. The user skips
measurements that feel pointless (pixel-area position, hole-to-glass gap,
standard pin lengths): design around the unknown instead (window = glass
minus a 0.5 lip; posts flattened 0.3 clear of the glass) and let the test
print check it. Ask for a measurement from edge to edge, never from a
hole's centre. Resoldering the TFT header (right-angle / wires) is on the
table if the case depth matters (~10 mm shallower).
Lessons from the TFT measuring and model (2026-09-29/30):
- Add up the numbers as they arrive: top gap + glass + bottom gap came out
  0.91 short of the board, and a direct "ear tops to glass bottom" showed
  the glass height (37.43) had missed the ~1 mm step at its top. Give the
  user the one direct measurement to recheck, not a list of suspects.
- When photos show geometry (the ears, the notch, the flat cable wrapping
  to the back), put it in the stand-in right away: the user caught that
  the model still had a plain rectangle.
- A clash where two solids only touch (posts ending exactly at the PCB)
  fails the check: give stand-ins an `eps` gap. Find which pair overlaps
  by intersecting pairs in a scratch copy with the output section cut off
  (else the assembly renders too and nothing is ever empty), then
  `bbox` the STL.
- Chain `export.sh && git commit`: one commit went out with a failing
  clash because the two ran side by side.
- Screw lengths: check them against what's in front of the pilot (an
  M2 x 6 through the 1.21 TFT board would have poked out of the front).
- Check tool access, not only clashes: draw the screwdriver along every
  screw's axis and intersect it with the printed parts (2026-10-02).
- Printed parts that touch (plate on rails, bosses on the base) need the
  same `eps` gap as stand-ins, or the part-vs-part clash check fails.
- "How does it go together" questions: three rounds of prose and ASCII
  drawings didn't land (2026-10-01); renders from the model with labels
  drawn over them did ("Mukklet Assembly",
  `enclosure/pages/assembly/assembly.html`, redone for three parts
  2026-10-02). Say first which parts there are and where each opens.
- The user wants such pages as local HTML files in the repo
  (`enclosure/pages/`, 2026-10-02), not claude.ai artifacts: write a
  standalone file and give its path. The two old published copies
  were deleted the same day.
- No headless browser works here (Brave hangs): to check a page's SVG
  labels, draw them onto the renders with PIL.
- OpenSCAD: a `module` can't be defined inside `if`/`else` (parser error
  with only a line number); define it at the top level.
- The mini's cradle is untested until printed (lip overhangs, the 0.2 gap
  under the shell's ledge, the tilt-in move was only worked out on paper:
  ~8 degrees fits under the lips, the back end clears the stops).
- The TFT's plug hanging down behind the screen is what limits the
  carrier: at 82 deep the EPLZON's front passes over it only at >= 8 mm
  standoffs, and its front middle M3 hole sits right over the plug.

## Next step
Milestone 3 (cover art) done. Enclosure (paused 2026-09-30, the user will
come back to it): the user built the cardboard mock-up at 50 x 82 x 62
(from the Miuzei-carrier templates; its floor map is out of date since the
EPLZON switch, reprint page 2 of `enclosure/cardboard.pdf` if needed).
Open, in order:
1. The mock-up's findings (size on the desk, screen tilt, knob reach,
   window vs the real TFT): not reported yet.
2. Wiring done (2026-10-02): the user made the cables and connected
   everything to the mini, "seems to work fine" (which way clockwise
   turns the volume was not said). For the TFT's signals at the mini they
   planned a 4-pin + a 2-pin housing, power apart (`docs/WIRING.md`).
3. Screws, all from the user's two self-tapping kits (lists in
   `enclosure/README.md`): M3 x 10 round head (base), 2.3 x 8 black pan
   head (front plate), 2 x 4 black pan head (TFT; try one in its 1.79
   holes first, else drill to 2.0).
4. Send `body`, `front`, `base` (and optionally `test_knob`) to the
   printing people (the user's plan, 2026-10-02), then go through the
   checklists in `enclosure/README.md`.
