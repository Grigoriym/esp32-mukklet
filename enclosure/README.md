# Enclosure

OpenSCAD model of the case, lifted from the desk display's
(`../esp32-desk-display/enclosure/`, the v2 "cut" carrier) and written from
`MEASUREMENTS.md`. Open `enclosure.scad` in OpenSCAD for the assembled view
(modules shown as coloured stand-in blocks, see-through gold = plugged-in
connectors). `-D cut=25` cuts the printed parts away left of x = 25.

```
enclosure/export.sh   # clash check, then stl/enclosure/*.stl (gitignored) + renders/enclosure-*.png
```

The clash check fails if a printed part overlaps a module stand-in, or two
stand-ins (with their plugs) overlap each other.

![front](renders/enclosure-front.png) ![back](renders/enclosure-back.png)
![inside](renders/enclosure-inside.png) ![section](renders/enclosure-section.png)

## Layout (first draft, 2026-09-29)

50 x 82 x 62 mm (W x D x H). A small wedge: the TFT, portrait, on a front
panel tilted 20° back over a 12 mm strip; the knob on top, centred behind
the screen; USB-C out the back.

- **TFT**: glass flat against the panel's inner face, the board screwed from
  behind onto 4 posts at its corner holes (the holes sit outside the glass
  on the seller's drawing). Pin header at the bottom, so the picture is
  upright with `FLIP_180 = 1` in `main/display.c`, as now. Its plug hangs
  down and back, in front of the carrier: that's what sets the case's depth.
- **Knob**: KY-040 flat under the top, shaft end to the front, pins to the
  back, pushed up into two snap hooks against 4 pads (as on the desk
  display). The cap goes on from outside afterwards.
- **Carrier**: the 4 x 6 cm Miuzei perfboard cut to 40 x 44 (17 hole rows),
  on 4 standoffs (5 mm). ESP32 front-to-back in two 15-pin female headers
  (8.5 mm), USB end at the back. Two JST-XH sockets in the free column left
  of the ESP32: TFT (8-pin) at the front, KNOB (5-pin) behind it. 8 pins
  at 2.5 mm on the 2.54 grid are 0.28 mm off at the ends: the pins take it.
- **Antenna**: the ESP32's antenna end is at the front, nothing above it;
  the TFT cable runs ~5 mm below it.
- **Vents**: top slots left and right of the knob, over the ESP32; low slots
  on the back beside the USB; slots in the floor under the ESP32. No
  sensors, so no sensor bay: only the ESP32's warmth to let out.
- **Base**: floor plate held by 4 × M3 screws from below into the shell's
  corner bosses; opens without touching the wiring.

## Parts to print

| STL | Qty | Orientation (as exported) | Notes |
|---|---|---|---|
| `shell` | 1 | upside down, top on the bed | check the USB hole's 13 mm bridge |
| `base` | 1 | flat | |
| `test_front` | 1 | like the shell | **print this first**: screen panel + top with the knob mount |

Material: PETG preferred (PLA softens ~55 °C). Fit clearance 0.3 mm (`clr`).

## Hardware

- 4 × M3 × 8 self-tapping (base → shell)
- 4 × M2 × 6 self-tapping (TFT → posts)
- 4 × M2 × 8 self-tapping (carrier → standoffs)
- 2 × 15-pin female headers, JST-XH sockets + cables: 8-pin (TFT), 5-pin (knob)
- 4 self-adhesive rubber feet

## Test print checklist (`test_front`)

1. TFT: the glass sits flat on the panel, no pixels cut off at the window's
   edges (the window is the glass minus a 0.5 mm lip; the pixel area's
   position wasn't measured), the screws pull the board flat.
2. KY-040: it snaps in, the shaft is centred in the 16 mm hole, and the cap
   turns and presses without rubbing the top.
