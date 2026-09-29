# Part measurements

The case model (`enclosure.scad`) is built from these numbers, in mm. The
ESP32 and KY-040 are the same parts as the desk display's and were measured
there (`../esp32-desk-display/enclosure/MEASUREMENTS.md`, 2026-09-28). The
TFT was measured 2026-09-29 with the "Mukklet Caliper Guide" artifact
(https://claude.ai/artifact/QTPxyAx3xMNtYf33rQmrr3); what was skipped comes
from the photos or standard parts, marked in `enclosure.scad`.

## TFT (1.69" ST7789V2, GERUI), held pins at the bottom, glass towards you
- Board width x height: 31.22 x 47.96 (2026-09-29; 48.01 the first time), widest points: the top
  edge has two ears with the top holes and a notch between them; the glass's
  flex cable (FP-169HSC01) wraps round the top edge through the notch to a
  connector on the back (photos 2026-09-29). "Top" = the ear tops.
- Back (photo): flex connector near the top, a few small SMD parts
  (C1 C2 R1-R3 U2) in the middle, rest flat; bottom holes beside the pin row
- Thickness, glass front to board back: 2.92; board alone (on an ear): 1.21; so the glass front is 1.71 above the board
- Glass size: 30.06 wide; 37.43 tall measured alone, but ear tops to glass top 4.73 and to glass bottom
  43.13 give 38.40: the 37.43 likely missed the ~1 mm step at the top. Model keeps 4.73..43.13 clear
- Notch between the ears: 18.91 wide, 5.13 deep from the ear tops (so the glass top, 4.73, overhangs it by 0.4)
- Glass position: ear tops to glass top 4.73, to glass bottom 43.13; bottom gap 4.94 (47.96 - 43.13 = 4.83,
  close enough); left = right by eye
- Picture (pixel area) 27.97 x 32.63 (seller). Its position under the glass is not measured
  (skipped 2026-09-29): the window is the glass minus a 0.5 lip, checked on the test print
- Mounting holes: diameter 1.79 measured (inside jaws read low, likely 2.0 for M2; try an M2 screw), centres 26.19 left-right x 42.91 top-bottom (measured)
- Front around the holes: not measured; the posts are flattened 0.3 mm clear of the glass instead
- Pin header: standard straight header on the back (photos), taken as 8.5 from the back surface
  incl. the plastic (not measured; resoldering to right-angle pins is an option if the depth matters); pin row centre ~2.0 above the bottom edge (photo)
- Tallest part on the back, header not counted: tiny SMDs and the flat cable soldered flat (photo); model keeps 2 clear

## ESP32 DevKit (ELEGOO ESP-32S, 30-pin), from the desk display
- Board 51.49 x 28.36, pin rows 25.4 apart, 15 pins each
- USB-C on the short edge opposite the antenna, port centre ~3.2 above the board bottom
- Tallest part on top 4.78 above the board bottom

## KY-040, from the desk display
- Board 26.18 x 19.29, holes 2.85 (16.48 apart)
- Board bottom to the cap's top 31.79, to the cap's lower edge 15.58
- Cap diameter 14.78; shaft centre ~8.6 from the short edge away from the pins
- Pins past the board's end: ~6, standard right-angle header (not measured)
