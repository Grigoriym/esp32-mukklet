# Enclosure

OpenSCAD model of the case, lifted from the desk display's
(`../esp32-desk-display/enclosure/`) and written from
`MEASUREMENTS.md`. Open `enclosure.scad` in OpenSCAD for the assembled view
(modules shown as coloured stand-in blocks, see-through gold = plugged-in
connectors). `-D cut=25` cuts the printed parts away left of x = 25.

```
enclosure/export.sh   # clash check, then stl/enclosure/*.stl (gitignored) + renders/enclosure-*.png
```

The clash check fails if a printed part overlaps a module stand-in, two
stand-ins (with their plugs) overlap each other, or two printed parts
overlap.

![front](renders/enclosure-front.png) ![back](renders/enclosure-back.png)
![inside](renders/enclosure-inside.png) ![section](renders/enclosure-section.png)
![exploded](renders/enclosure-exploded.png)

## Cardboard mock-up

```
enclosure/cardboard.py [cardboard mm, default 2]   # -> enclosure/cardboard.pdf (gitignored)
```

Two A4 pages of 1:1 templates, sized from the model (`part="dims"`): 2
sides, front strip, screen panel (window + the TFT board's outline), top
(knob hole + the KY-040's outline), back (USB hole), and a floor with the
ESP32 mini drawn on it (an outline, not a hole). Print at
100% ("Actual size") and check the 50 mm bar with a ruler. Panels other
than the sides are narrower by 2 × the cardboard thickness, so they fit
between the sides. Checks: overall size on the desk, the screen's tilt and
readability, knob reach, and whether the window lines up with the real
TFT taped behind it.

## Layout (three parts since 2026-10-02)

50 x 82 x 62 mm (W x D x H). A small wedge: the TFT, portrait, on a front
panel tilted 20° back over a 12 mm strip; the knob on top, centred behind
the screen; USB-C out the back.

Three printed parts: the **body** (side walls, top, back; open at the
bottom and the front), the **front plate** (the tilted screen panel, a
flat plate that drops in between the side walls) and the **base** (floor,
the strip under the screen, the ESP32's cradle). Why the plate is its own
part: in the one-piece shell a screwdriver couldn't reach the TFT's two
top screws (their line ran into the back wall, 34 mm up; from the open
bottom it was 24° off the screw). Now the TFT is screwed on while the
plate lies on the desk. It also means a wrong window costs one small
plate, not the whole case, and the plate prints face down.

- **Front plate**: lies on a rail along each side wall (6 wide, 8 deep,
  floor to top) and is held by 4 screws from the front, heads visible at
  its corners. Its bottom edge sits 0.2 above the base's strip.
- **TFT**: glass flat against the plate's inner face, the board screwed from
  behind onto 4 posts at its corner holes, flattened on the glass side
  (0.3 mm clear of it). Pin header at the bottom, so the picture is
  upright with `FLIP_180 = 1` in `main/display.c`, as now. Its plug hangs
  down and back, ending ~12 mm in front of the ESP32.
- **Knob**: KY-040 flat under the top, shaft end to the front, pins to the
  back, pushed up into two snap hooks against 4 pads (as on the desk
  display). The cap goes on from outside afterwards.
- **ESP32 mini** (D1 mini layout, 31.51 x 39.02, no mounting holes): flat
  on the floor at the back, metal can up, USB end at the back wall, 3 mm
  above the floor (room for the solder joints). Held in a cradle on the
  base, no screws and nothing that flexes: the antenna end slides under
  two lips at the front corners, the back drops onto two rests with a
  stop behind them, side guides beside the pin rows, and a ledge on the
  body's back wall sits 0.2 above the USB socket once the base is
  screwed on. Pin headers point up, on both inner rows and the outer row
  on the side away from the RST button; the TFT and knob cables plug onto
  them with female Dupont housings (tops ~24 mm above the floor, the
  knob's board is at 48). Kept at 82 deep for the cardboard mock-up
  already built; the USB hole is 16 mm lower than on that mock-up's back.
- **Antenna**: the ESP32's antenna end faces the front, in the middle of
  the case; nothing above it but the knob's board, 40 mm up.
- **Vents**: top slots left and right of the knob, over the ESP32; low slots
  on the back beside the USB; slots in the floor under the ESP32. No
  sensors, so no sensor bay: only the ESP32's warmth to let out.
- **Base**: floor plate with the 12 mm strip under the screen standing on
  its front edge (on the body it would be a 46 mm bridge when printed),
  held by 4 × M3 screws from below into the body's corner bosses. The
  ESP32 is on the base, the TFT on the plate and the knob in the body, so
  the cables need enough slack to lay the three beside each other.

Assembly order: TFT onto the plate (4 screws from behind), its cable on;
knob snapped into the body; plate onto the body (4 screws from the front);
ESP32 into the cradle, cables on; base on (4 screws from below).

## Parts to print

| STL | Qty | Orientation (as exported) | Notes |
|---|---|---|---|
| `body` | 1 | upside down, top on the bed | check the USB hole's 13 mm bridge |
| `front` | 1 | screen face on the bed | the window's edge is chamfered: no supports |
| `base` | 1 | flat | the two lips over the front corners are 2.5 mm overhangs |
| `test_knob` | 1 | like the body | optional: the top's front with the knob mount, to check it before the whole body |

Material: PETG preferred (PLA softens ~55 °C). Fit clearance 0.3 mm (`clr`).

## Hardware

Two self-tapping kits, both the user's: silver stainless (M3 × 6/10/14/18,
M4, M5, M6; flat and round head) and small black pan head (2 × 4/5/6,
2.3 × 5/8/10, 2.6 × 6, 3 × 4/5/6).

- 4 × M3 × 10, round head, silver kit (base → body; pilot 2.5)
- 4 × 2.3 × 8, black kit (front plate → rails; 2.7 hole in the plate,
  pilot 1.9, 6.5 deep)
- 4 × 2 × 4, black kit (TFT → posts, pilot 1.6; a 6 would poke out the
  front). The TFT's holes measured 1.79: try one screw first, else drill
  them to 2.0
- 3 × 10-pin male headers on the ESP32 mini, pins up; cables to the TFT
  (8 wires, 12 cm) and knob (5 wires, 15 cm) with female Dupont housings
  on both ends: at the mini a 5-pin + a 3-pin + a 4-pin (which pad goes
  where: `docs/WIRING.md`); the TFT's VCC and the knob's + share the one
  3V3 pin (two wires in one terminal)
- 4 self-adhesive rubber feet

## Front plate checklist (`front`)

1. TFT: the glass sits flat on the plate, no pixels cut off at the window's
   edges (the window is the glass minus a 0.5 mm lip; the pixel area's
   position wasn't measured), the screws pull the board flat.
2. On the body: the plate drops in between the side walls (0.3 each side),
   lies flush with their edges, and its 4 holes meet the rails' pilots.

## Knob checklist (`test_knob` or `body`)

1. KY-040: it snaps in, the shaft is centred in the 16 mm hole, and the cap
   turns and presses without rubbing the top.

## Base print checklist

0. The strip under the screen stands 0.2 below the plate's bottom edge and
   flush with the side walls' front.
1. ESP32 mini: the antenna end slides under the two lips at a slight tilt
   and the back drops in front of the stops; no play left-right.
2. The soldered pins' stubs underneath don't touch the floor (3 mm room).
3. With the base screwed into the body: the USB socket is centred in the
   hole, a cable plugs in fully, and the board's back end can't lift.

## Pictures and artifacts

`-D explode=45 -D explode_front=30` lifts the body off the base and pulls
the front plate (with the TFT) forward;
`-D mini_tilt=8 -D mini_back=6` shows the mini on its way into the cradle.
`artifacts/` holds the sources of the two claude.ai pages, since the pages
themselves are the only other copy:

- `artifacts/caliper.html`: "Mukklet Caliper Guide",
  https://claude.ai/artifact/QTPxyAx3xMNtYf33rQmrr3
- `artifacts/assembly/`: "Mukklet Assembly" (5 renders with labels drawn
  over them), https://claude.ai/artifact/TWDPQ22zmgzkANerb1wxgc. **Out of
  date since the three-part split (2026-10-02)**: it shows the one-piece
  shell, and `show_shell` is now `show_body`. Renders:
  `openscad -D show_labels=false --colorscheme=Tomorrow --imgsize=1000,750`
  plus `-D explode=55 --camera=25,41,55,65,0,150,330` (a1),
  `-D show_shell=false -D explode=200 -D mini_tilt=8 -D mini_back=6
  --camera=25,58,8,60,0,140,150` (a2; a3 without the tilt),
  `--camera=25,41,34,65,0,150,290` (a5),
  `-D cut=25 --projection=o --camera=25,41,34,90,0,270,230` (a6).
