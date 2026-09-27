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
- **Target screen (not bought yet)**: 1.69" IPS 240×280 **ST7789** SPI
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
- **Planned TFT wiring**: SCL D18, SDA D23, RES GPIO17 (TX2), DC GPIO16
  (RX2), CS D5, BLK D4. Keeps I2C D21/D22 and the knob D25/D26/D27 free,
  avoids D2 (onboard LED).
- **Knob**: KY-040 on D25 (CLK) / D26 (DT) / D27 (SW), as on the desk
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
- **This board: MAC `70:4b:ca:4d:f0:1c`** (ESP32-D0WD-V3 rev 3.1, 40 MHz
  crystal). Read-only identity check before flashing:
  `esptool -p /dev/ttyUSB0 chip-id`. The weather station's board is a
  different one; if it's online, `desk.local` answers over WiFi.
- When it arrived here it ran unknown firmware that printed unreadable
  bytes at **every** baud rate (57600-921600, incl. 115200/74880), yet
  esptool connected fine: the garbage was that firmware, not the wiring or
  the USB link. Flashing replaced it. If a board's serial log looks like
  noise, try `esptool chip-id` before suspecting the wiring.

## Firmware (milestone 1: text only, 2026-09-27)
WiFi + mDNS `mukklet.local` + WebSocket server (`link.c`) + OLED text +
knob commands. `hello` asks for cover `"format": "none"`, so Mukk sends no
art yet. Pure, host-tested modules (no ESP-IDF): `proto` (parse/build
messages), `player` (session, position extrapolation, 15 s offline rule),
`ui` (whole screen from player state + time, stateless incl. scrolling),
`fb` (framebuffer in SSD1306 page layout), `font` (UTF-8, 6x10),
`gesture` (single/double/long press), `encoder_decode` (lifted).
- **Font**: X11 misc-fixed 6x10 (public domain), ASCII + Latin-1/2/9/13/15
  + Cyrillic, generated into `main/font_data.c` by `tools/gen_font.py`
  (needs Pillow; rerun, don't hand-edit). 21 chars per line; longer lines
  scroll. Unknown characters draw as a box.
- **Layout** (128x64): title / artist / album (year) / "Next: …" /
  progress bar / status icon, position, volume %, duration.
- **Knob**: turn = volume ±5 per detent (a fast spin is one `cmd`), press =
  play/pause (sent 300 ms after release, waiting for a double), double =
  next, long (600 ms) = prev.
- IDF 6 gotcha: the WS handler is **not** called for the handshake any
  more; `hello` goes out from `ws_post_handshake_cb`
  (`CONFIG_HTTPD_WS_POST_HANDSHAKE_CB_SUPPORT`).
- Verified on the board 2026-09-27 with `tools/fake_mukk.py` (hello, track,
  heartbeats, newest-client-wins, knob turns both ways: CW = volume up),
  then with the **real Mukk** (its DisplayLink): screen and knob (volume,
  press/double/long) work, per the user.

## Open questions
- Which fields to show (the OLED prototype is meant to answer this).

## Build / flash
ESP-IDF, same setup as the desk display:
`. ~/esp/esp-idf/export.sh && idf.py -p /dev/ttyUSB0 build flash`.
If the weather station is plugged in too, the ports shift (`ttyUSB1`):
check `ls /dev/ttyUSB*` and the MAC above before flashing.
Boot logs (`idf.py monitor` needs a TTY the harness doesn't have):
`tools/serial_log.py` resets the board and captures, run with the IDF
python env,
`~/.espressif/python_env/idf6.2_py3.14_env/bin/python tools/serial_log.py <seconds> "<regex>"`
(port hard-coded to `/dev/ttyUSB0`).
Checks that need hands (turning the knob): run the capture with
`run_in_background`, tell the user what to do, read the output when it ends.

Checks (same as CI, `.github/workflows/ci.yml`; IDF env sourced):
`tools/test.sh` (host unit tests; `SHOW_ART=1` prints the rendered screens
as ASCII art), `tools/format.sh --check`, `tools/lint.sh`,
`tools/size_check.sh`. WiFi credentials: `main/wifi_secrets.h` (gitignored,
template `.example`).

Playing Mukk's role: `python3 tools/fake_mukk.py [--host IP] [--seconds N]`
connects to the display, sends a small playlist (Cyrillic, long title, no
tags, unknown duration) and obeys the knob, printing each `cmd`.

## Next step
Use it with Mukk for a while and decide which fields earn their place.
No cover art on the OLED (decided 2026-09-27): covers wait for the ST7789;
until then Mukk tests its cover code against `tools/fake_display.py`.
