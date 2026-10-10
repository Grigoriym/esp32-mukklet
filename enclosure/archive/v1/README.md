# Mukklet case v1 (printed 2026-10-07)

The version that was printed and assembled. Kept for reference; the
current design is `../../enclosure.scad` (v2).

- `enclosure.scad`: the model as printed (commit 48c3eaf). It runs on its
  own in OpenSCAD (`openscad enclosure.scad`); `../../export.sh` is for
  the current model only.
- `renders/`: its pictures.
- `pictures/`: v1 next to the v2 change. `base-*-v1-vs-v2.png`: the base
  inside the walls (holes 0.2 from its edge once drilled) vs over the
  whole footprint. `knob-hooks-section.png`: v1's snap hooks, cut through.
  `knob-nut-idea-section.png`: the nut idea, drawn with guessed sizes
  before the KY-040 was measured.

## What the first print showed

Printed by a local FDM shop, assembled by the user. Works as a case; the
knob mount needs a v2.

- **Base / cradle: works.** The shop printed support under the two lips
  (expected, they're overhangs); the user cut it out with a craft knife
  heated with a lighter. The board slides in, lies flat, no play; USB
  centred in the hole, a cable plugs in fully, the back end stays down.
- **Base screws:** the silver kit's "M3" measures 3.64 across the thread
  (~11.7 long): it doesn't pass the 3.4 holes, so the user drilled them
  to 4 mm. Into the bosses' 2.5 pilots it went hard but held.
- **Front plate: works.** TFT screwed on (2 x 4 fine), plate on the body.
  The 2.3 x 8 screws were hard to drive into the 1.9 pilots and may not
  be all the way in. Taking the plate off (2026-10-10), two of them
  snapped under the head and left their shanks in the rails. Likely
  cause: the 1.9 pilot prints smaller (FDM holes come out ~0.2-0.4
  undersized), so a 2.3 screw cuts a lot of plastic, and the pilot is
  only 0.5 deeper than the screw reaches; the small black screws are
  hardened and snap at the neck rather than twist. The user got the
  shanks out; 2 x 6 screws from the same kit went in easily into all
  four holes (already cut by the 2.3s) and hold the plate for now.
- **Knob: doesn't hold.**
  1. The KY-040's header had its plastic strip on the component face;
     the model assumed the pins stick out past the board's end, so the
     pad near that end hit it. The user removed the header (to go back on
     the underside).
  2. The left hook doesn't catch the board's edge (the right one does):
     pressing the knob pushes the board down; for now the user holds a
     balance. The barbs reach only 0.8 under the edge, and the board's
     thickness (1.6) was assumed.
  3. Where the hooks put the board, the shaft isn't centred in the 16 mm
     hole; centring it needs the board pushed right, which the hooks
     don't allow.
