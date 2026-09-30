# Enclosure

OpenSCAD model of the case, lifted from the desk display's
(`../esp32-desk-display/enclosure/`) and written from
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

## Cardboard mock-up

```
enclosure/cardboard.py [cardboard mm, default 2]   # -> enclosure/cardboard.pdf (gitignored)
```

Two A4 pages of 1:1 templates, sized from the model (`part="dims"`): 2
sides, front strip, screen panel (window + the TFT board's outline), top
(knob hole + the KY-040's outline), back (USB hole), and a floor with the
ESP32, carrier and its screw holes drawn on it (outlines, not holes). Print at
100% ("Actual size") and check the 50 mm bar with a ruler. Panels other
than the sides are narrower by 2 × the cardboard thickness, so they fit
between the sides. Checks: overall size on the desk, the screen's tilt and
readability, knob reach, and whether the window lines up with the real
TFT taped behind it.

## Layout (first draft, 2026-09-29)

50 x 89 x 62 mm (W x D x H). A small wedge: the TFT, portrait, on a front
panel tilted 20° back over a 12 mm strip; the knob on top, centred behind
the screen; USB-C out the back.

- **TFT**: glass flat against the panel's inner face, the board screwed from
  behind onto 4 posts at its corner holes, flattened on the glass side
  (0.3 mm clear of it). Pin header at the bottom, so the picture is
  upright with `FLIP_180 = 1` in `main/display.c`, as now. Its plug hangs
  down and back, in front of the carrier: that's what sets the case's depth.
- **Knob**: KY-040 flat under the top, shaft end to the front, pins to the
  back, pushed up into two snap hooks against 4 pads (as on the desk
  display). The cap goes on from outside afterwards.
- **Carrier**: EPLZON 38.1 x 50.8 breadboard-style PCB (sizes from the
  seller's drawing), on 2 standoffs (5 mm) at its middle M3 holes plus 4
  rests under its M2 corner holes. ESP32 front-to-back in two 15-pin female
  headers (8.5 mm) in columns A and I (10 pitches apart, as its pins), rows
  3-17, USB end at the back. Each row's A-E and F-J holes are joined
  underneath, so every ESP32 pin has 4 spare holes: the TFT and knob wires
  are soldered into those (under the ESP32, before it's plugged in), with
  Dupont housings on the module ends. No JST sockets: the joined rows
  would short their pins. The carrier sets the depth: it has to stay behind
  the TFT's plug and in front of the back screw bosses (88 is the minimum,
  89 leaves 1 mm).
- **Antenna**: the ESP32's antenna end is at the front, nothing above it;
  the carrier's rows 1-2 are under it (copper pads only).
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

The M3 screws come from the user's self-tapping kit (M3-M6, round and flat
head; smallest M3 × 6). The screen's M2 screws come from a separate small
self-tapping kit (M2 × 4/5/6, M2.3, M2.6, M3 × 4-6, pan head).


- 4 × M3 × 10 self-tapping, round head (base → shell)
- 4 × M2 × 4 self-tapping, pan head (TFT → posts; a 6 would poke out the
  front). The TFT's holes measured 1.79: try one screw first, else drill
  them to 2.0
- 2 × M3 × 6 self-tapping, round head (carrier's middle holes → standoffs)
- 2 × 15-pin female headers; cables to the TFT (8 wires) and knob (5 wires),
  soldered on the carrier, Dupont housings on the module ends
- 4 self-adhesive rubber feet

## Test print checklist (`test_front`)

1. TFT: the glass sits flat on the panel, no pixels cut off at the window's
   edges (the window is the glass minus a 0.5 mm lip; the pixel area's
   position wasn't measured), the screws pull the board flat.
2. KY-040: it snaps in, the shaft is centred in the 16 mm hole, and the cap
   turns and presses without rubbing the top.
