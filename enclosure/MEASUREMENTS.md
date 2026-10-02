# Part measurements

The case model (`enclosure.scad`) is built from the parts' measured
dimensions, in mm. The numbers live in the shared part sheets
(`../grappim-watcher/docs/esp32/parts/`), one per part, under "Mechanical";
a new measurement of a part goes there, not here.

| Part | Sheet | Measured |
|---|---|---|
| TFT 1.69" ST7789V2 | `tft-st7789v2.md` | 2026-09-29 |
| ESP32 mini | `esp32-mini.md` | 2026-10-01 |
| KY-040 | `ky-040.md` | 2026-09-28, for the desk display |

The TFT and the mini were measured with the "Mukklet Caliper Guide"
(`pages/caliper.html`). What was skipped comes from the photos or standard
parts and is marked in `enclosure.scad`.

## What the model does with them
- TFT glass: keeps 4.73..43.13 below the ear tops clear; the window is the
  glass minus a 0.5 lip (the pixel area's position isn't measured), checked
  on the printed front plate.
- TFT posts: flattened 0.3 clear of the glass (the front around the holes
  isn't measured).
- TFT back: 2 clear for the SMDs and the flat cable; the header is taken as
  8.5 from the back surface.
- Mini: the standard D1 mini pin grid, a 6 mm strip free under each long
  side for the solder joints, supports only under the first ~6 mm at the
  antenna end and the middle.
