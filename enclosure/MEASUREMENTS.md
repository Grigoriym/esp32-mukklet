# Part measurements

The case model (`enclosure.scad`) is built from these numbers, in mm. The
ESP32 and KY-040 are the same parts as the desk display's and were measured
there (`../esp32-desk-display/enclosure/MEASUREMENTS.md`, 2026-09-28). The
TFT is still to measure: the "Mukklet Caliper Guide" artifact
(https://claude.ai/artifact/QTPxyAx3xMNtYf33rQmrr3) walks through it one
step at a time. Until then the model uses the seller's drawing and guesses,
marked `ASSUMED` in `enclosure.scad`.

## TFT (1.69" ST7789V2, GERUI), held pins at the bottom, glass towards you
- Board width x height: 31.22 x 48.01 (2026-09-29), widest points: the top
  edge has two ears with the top holes and a notch between them; the glass's
  flex cable (FP-169HSC01) wraps round the top edge through the notch to a
  connector on the back (photos 2026-09-29). "Top" = the ear tops.
- Back (photo): flex connector near the top, a few small SMD parts
  (C1 C2 R1-R3 U2) in the middle, rest flat; bottom holes beside the pin row
- Thickness, glass front to board back: _ ; board alone: _ (model guesses 1.6 + 1.8)
- Glass size: _ (model guesses 30.07 x 37.43, the usual 1.69" panel)
- Glass position: top gap _, bottom gap _, left gap _ (model: centred left-right, top gap 5.3)
- Picture (lit area) 27.97 x 32.63 (seller); position from the cover art on screen
  (cover = 160 px at x 40, y 8): board top edge to cover top _, board left edge to
  cover left _, cover width _ (expect ~18.6)
- Mounting holes: diameter _ (model 2.2, M2), centres 26 left-right x 43 top-bottom (seller)
- Front around the holes clear of the glass's cable and parts: _ (the case puts screw posts there)
- Pin header: pins come out of the back (photos), _ long from the back surface incl. the plastic
  (model 8.25); pin row centre _ above the bottom edge (model 2.0)
- Tallest part on the back, header not counted: _ (model 2)

## ESP32 DevKit (ELEGOO ESP-32S, 30-pin), from the desk display
- Board 51.49 x 28.36, pin rows 25.4 apart, 15 pins each
- USB-C on the short edge opposite the antenna, port centre ~3.2 above the board bottom
- Tallest part on top 4.78 above the board bottom

## KY-040, from the desk display
- Board 26.18 x 19.29, holes 2.85 (16.48 apart)
- Board bottom to the cap's top 31.79, to the cap's lower edge 15.58
- Cap diameter 14.78; shaft centre ~8.6 from the short edge away from the pins
- Pins past the board's end: _ (model 6)
