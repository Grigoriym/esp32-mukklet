// Mukklet enclosure: a small wedge, the 1.69" ST7789V2 TFT (portrait) on
// the front panel tilted back, the KY-040 knob on top, USB-C out the back.
// Lifted from ../esp32-desk-display/enclosure/enclosure_v2.scad. The ESP32
// plugs front-to-back into two female headers on an EPLZON 38.1 x 50.8
// breadboard-style carrier; module cables are soldered to it. No sensors, so no
// sensor bay and no hood: only the ESP32's own warmth to vent.
// Sizes in mm, from MEASUREMENTS.md; values marked ASSUMED were not
// measured (the TFT's are the seller's, not ours, until measured).
//
// Axes: X = width, left to right seen from the front; Y = depth, front at
// y = 0; Z = up, table at z = 0 (rubber feet not modelled).
//
// Printed parts:
//   shell - walls, tilted screen panel with the TFT's 4 screw posts, top
//           with the knob's snap hooks; open at the bottom
//   base  - floor plate with the carrier's standoffs, 4 screws up into the
//           shell's corner bosses
// Pick one with -D 'part="shell"' (see export.sh). Single parts come out in
// print orientation, "assembly" shows everything in place with stand-in
// blocks for the modules, "clash" is empty when nothing overlaps.

part = "assembly"; // [assembly, shell, base, test_front, clash, dims]
cut = -1; // assembly only: >= 0 cuts the printed parts away left of this x
show_shell = true; // assembly only: untick to see inside
show_base = true;
show_labels = true; // names over the module stand-ins

$fn = 48;
eps = 0.01;

// ------------------------------------------------------------ print
clr = 0.3; // fit clearance between printed parts / around boards

// ------------------------------------------------------------ modules
// TFT, portrait, pin header on the bottom edge (upright with FLIP_180 = 1
// in main/display.c, as it is now)
tft_w = 31.22; // across the two ears at the top (the widest part)
tft_h = 47.96; // ear tops to the bottom edge (48.01 the first time). The top edge has a notch
              // between the ears (holes on the ears); the glass's flex cable
              // wraps through it to a connector on the back (photos 2026-09-29)
tft_pcb_t = 1.21; // measured on an ear
tft_front = 2.92 - tft_pcb_t; // PCB front face to the glass front (2.92 glass front to board back)
tft_glass_w = 30.06; // measured, incl. the step at the top; centred left-right (by eye)
tft_glass_top = 4.73; // ear tops to the glass's top edge (measured; bottom gap 4.94 ~ 47.96 - 43.13)
tft_glass_h = 43.13 - tft_glass_top; // ear tops to the glass bottom 43.13 (measured). The
                                     // glass alone measured 37.43: that missed ~1 mm, likely the step
                                     // at the top; this keeps the whole glass stack out
tft_lit_w = 27.97; // pixel area, seller; its position under the glass isn't measured:
tft_lit_h = 32.63; // the window is sized from the glass instead (tft_lip)
tft_lip = 0.5; // the panel covers this much of the glass's edge all round
tft_hole_x = 26.19; // hole centres, measured; centred on the board (ASSUMED)
tft_hole_z = 42.91;
tft_hole_d = 1.79; // measured with the inside jaws, which read low (the OLED's 1.70 was
                   // likely 2.0): likely 2.0, for M2; if an M2 screw won't pass, M1.7
tft_hdr_edge = 2.0; // pin row's centre above the bottom edge (standard header, from the photo)
tft_pins = 8.5; // standard straight header on the back: 2.5 plastic + ~6 pin (not measured)
tft_dupont = 14; // a plugged-in 8-pin Dupont housing adds this
tft_back = 2; // parts on the PCB's back: tiny SMDs, flat cable soldered flat (photo; generous)
tft_notch_w = 18.91; // notch between the ears at the top
tft_notch_d = 5.13; // ear tops to the notch's bottom edge: the glass's top step
                    // (4.73) overhangs it by 0.4
tft_flex_w = 14; // flat cable, wraps round the notch's edge to the back (ASSUMED, photos)
tft_flex_bulge = 0.8; // its bend above the glass's top edge (ASSUMED)
tft_flex_conn = 1.2; // its connector on the back: height (ASSUMED)
tft_flex_conn_l = 7; // and length down from the notch (ASSUMED, photos)

esp_l = 51.49;
esp_w = 28.36;
esp_t = 1.6; // ASSUMED
esp_top = 4.78; // tallest part above the board bottom
esp_usb_z = 3.2; // USB-C centre above the board bottom
esp_pin_span = 15 * 2.54; // pin rows' length, centred on the board (ASSUMED)
esp_antenna = 6; // antenna end, opposite the USB

ky_l = 26.18;
ky_w = 19.29;
ky_t = 1.6; // ASSUMED
ky_cap_d = 14.78;
ky_shaft_y = 16 - ky_cap_d / 2; // shaft centre from the board's short edge
                                // away from the pins (desk display, +-1)
ky_cap_h = 31.79 - 15.58; // cap height
ky_cap_above = 15.58 - ky_t; // board front face to the cap's lower edge
ky_body = 12.5; // EC11 body footprint (ASSUMED, standard part)
ky_body_h = 7; // EC11 body height above the board (ASSUMED)
ky_pins = 6; // right-angle pins past the board's end (ASSUMED)
ky_dupont = 14; // a plugged-in 5-pin Dupont housing adds this

// carrier: EPLZON 38.1 x 50.8 breadboard-style PCB (seller's drawing, not
// measured). 17 rows; columns A-E and F-J, each row's A-E and F-J joined
// underneath, a 3-pitch gap between E and F with two M3 holes (40.6 apart,
// in rows 1 and 17) and four M2 corner holes (31.8 x 44.5). Column A to I
// is 10 pitches = the ESP32's pin rows, so its headers go in A and I and
// every pin gets 4 joined holes (B-E / F-H) to solder the module wires into
p = 2.54;
perf_w = 38.1;
perf_d = 50.8;
perf_t = 1.6; // ASSUMED
perf_standoff = 9; // high enough that the carrier's front passes over the TFT's plug
                   // (8 is the minimum at D = 82; 9 leaves 1 mm)
perf_m2 = [31.8, 44.5]; // corner holes: the front two take M2 x 6 into standoffs,
                        // the back two sit on plain rests
perf_m3 = 40.6; // centre holes: only the back one takes an M3 x 6; the
                // front one is over the TFT's plug
hdr_h = 8.5; // female header the ESP32 plugs into (ASSUMED: standard)

// ------------------------------------------------------------ case
wall = 2.0;
tilt = 20; // screen panel, back from vertical
skirt_h = 12; // strip under the screen: lifts it off the desk, room for the TFT's plug
panel_margin = 3; // screen panel beyond the TFT board, top and bottom
W = 50;
D = 82; // kept for the cardboard mock-up already built (2026-09-30)
base_t = 3;

panel_len = tft_h + 2 * panel_margin; // along the slope
H = skirt_h + panel_len * cos(tilt);
run = panel_len * sin(tilt); // how far back the panel's top edge is

boss_d = 7;
boss_pilot = 2.5; // M3 x 10 self-tapping, round head (the user's kit), 10 deep
boss_in = wall + boss_d / 2 - 1; // centre from the outside; sunk 1 mm into both walls
boss_xy = [[boss_in, boss_in], [W - boss_in, boss_in], [boss_in, D - boss_in], [W - boss_in, D - boss_in]];

// TFT: glass flat against the panel's inner face, the PCB screwed from
// behind onto 4 posts at its corner holes (4 x M2 x 4 self-tapping: a 6
// would poke out of the front). The
// holes sit outside the glass (seller's drawing), so posts fit there
tft_v = panel_len / 2; // board centre, up the slope from the skirt's edge
tft_post_d = 4.5;
tft_glass_gap = 0.3; // posts to the glass
tft_pilot = 1.6;
tft_glass_z0 = tft_h / 2 - tft_glass_top - tft_glass_h; // glass bottom edge, from the board centre
// window: the glass minus the lip, centred on the glass. Leaves 0.5 spare
// round the pixel area left/right, ~2.4 top/bottom, if it's centred: the
// test print shows whether pixels get cut off
lit_z = tft_glass_z0 + tft_glass_h / 2; // window centre above the board centre
win_w = tft_glass_w - 2 * tft_lip;
win_h = tft_glass_h - 2 * tft_lip;

// knob on top, centred: KY-040 board flat under the top, shaft end to the
// front, pins to the back; held by two snap hooks on its long edges,
// pushed up against 4 pads
knob_x = W / 2;
ky_y0 = 25; // board's front edge
knob_y = ky_y0 + ky_shaft_y;
knob_gap = 1; // top surface to the cap's lower edge
knob_hole = 16;
ky_face_z = H + knob_gap - ky_cap_above; // board front (component) face
ky_back_z = ky_face_z - ky_t;

// carrier and ESP32 on the carrier's 2.54 hole grid: hole (col, row) is at
// [grid_x0 + col * p, grid_y0 + (row - 1) * p]; cols A-E = 0-4, F-J = 7-11
// across X, rows 1-17 along Y. ESP32 pins in cols A (0) and I (10), rows
// 3-17, USB end at the back; rows 1-2 are under its antenna end
usb_gap = 3.6; // ESP32's USB end to the back wall; keeps the carrier off the back bosses
esp_pin_edge = (esp_l - esp_pin_span) / 2 + p / 2; // board end to the first pin's centre
esp_row_edge = (esp_w - 10 * p) / 2; // long edge to its pin row's centre
perf_x0 = (W - perf_w) / 2;
grid_x0 = perf_x0 + (perf_w - 11 * p) / 2; // column A; grid ASSUMED centred on the board
grid_y0 = D - wall - usb_gap - esp_l + esp_pin_edge - 2 * p; // row 1
perf_y0 = grid_y0 - (perf_d - 16 * p) / 2;
perf_x1 = perf_x0 + perf_w;
perf_y1 = perf_y0 + perf_d;
perf_z = base_t + perf_standoff;
esp_x0 = grid_x0 - esp_row_edge;
esp_y0 = grid_y0 + 2 * p - esp_pin_edge;
esp_x1 = esp_x0 + esp_w;
esp_y1 = esp_y0 + esp_l;
esp_z = perf_z + perf_t + hdr_h; // ESP32 board bottom
usb_x = (esp_x0 + esp_x1) / 2;
usb_z = esp_z + esp_usb_z;
usb_hole = [13, 8];

echo(str("case ", W, " x ", D, " x ", H, " mm (W x D x H)"));
echo(str("usb centre z ", usb_z, ", knob board face z ", ky_face_z));
assert(perf_x1 < W - wall && perf_y1 < D - boss_in - boss_d / 2, "carrier hits a wall or a back boss");
assert(norm([W - boss_in - esp_x1, D - boss_in - esp_y1]) > boss_d / 2 + 0.5 || esp_x1 < W - boss_in - boss_d / 2 - 0.5, "ESP32 hits the back-right boss");
assert(esp_x1 < W - wall && esp_y1 < D - wall, "ESP32 hits a wall");
assert(tft_hole_z / 2 - tft_pilot / 2 - 0.6 > tft_glass_z0 + tft_glass_h + tft_glass_gap, "TFT post flats cut into the screw holes");
assert(tft_hole_x / 2 - tft_post_d / 2 > tft_notch_w / 2, "TFT top posts off the ears");
assert(win_w > tft_lit_w && win_h > tft_lit_h, "window smaller than the pixel area");

// no JST sockets: each row's joined holes would short their pins. The
// TFT and knob cables are soldered into the ESP32 pins' spare holes
// (under the ESP32, before it's plugged in), Dupont housings on the module ends

// ------------------------------------------------------------ helpers
module profile() polygon([[0, 0], [0, skirt_h], [run, H], [D, H], [D, 0]]);

module extrude_x(x0, w) translate([x0, 0, 0]) rotate([90, 0, 90]) linear_extrude(w) children();

module outer() extrude_x(0, W) profile();

// inside of the shell, open at the bottom
module cavity() extrude_x(wall, W - 2 * wall) hull() {
  offset(delta = -wall) profile();
  translate([0, -wall - 1]) offset(delta = -wall) profile();
}

// panel coordinates: x across (0 = centre), y inwards (0 = outer face),
// z up the slope (0 = the skirt's top edge)
module panel_frame() translate([W / 2, 0, skirt_h]) rotate([-tilt, 0, 0]) children();

// at the TFT board's centre, on the panel's inner face (= the glass front)
module tft_frame() panel_frame() translate([0, wall, tft_v]) children();

module box(p0, p1) translate(p0) cube(p1 - p0);

module slots(x0, x1, y0, y1, z0, z1, pitch = 5, w = 2) {
  n = floor((x1 - x0 - w) / pitch);
  for (i = [0:n]) box([x0 + i * pitch, y0, z0], [x0 + i * pitch + w, y1, z1]);
}

module stadium(w, h, len) // along y, centred in x/z
  hull() for (s = [-1, 1]) translate([s * (w - h) / 2, 0, 0]) rotate([-90, 0, 0]) cylinder(d = h, h = len);

tft_holes = [for (sx = [-1, 1], sz = [-1, 1]) [sx * tft_hole_x / 2, sz * tft_hole_z / 2]];

// ------------------------------------------------------------ shell
// posts flattened on the glass side, tft_glass_gap clear of it, so they fit
// whatever the holes' exact position
module tft_mount() tft_frame() difference() {
  for (h = tft_holes) translate([h.x, -0.5, h.y]) rotate([-90, 0, 0]) cylinder(d = tft_post_d, h = tft_front + 0.5);
  g = tft_glass_gap;
  box([-tft_glass_w / 2 - g, -1, tft_glass_z0 - g], [tft_glass_w / 2 + g, tft_front + 1, tft_glass_z0 + tft_glass_h + g]);
}

module tft_pilots() tft_frame() for (h = tft_holes)
  translate([h.x, tft_front + eps, h.y]) rotate([90, 0, 0]) cylinder(d = tft_pilot, h = tft_front + wall - 0.8);

// lit area plus 0.5 all round, chamfered 45 degrees outwards
module tft_window() tft_frame() translate([0, 0, lit_z]) hull() {
  translate([0, 0.5, 0]) cube([win_w, 1, win_h], center = true);
  translate([0, -wall - 0.5, 0]) cube([win_w + 2 * (wall + 1), 1, win_h + 2 * (wall + 1)], center = true);
}

module bosses() for (p = boss_xy) translate([p.x, p.y, base_t]) cylinder(d = boss_d, h = H);

module boss_pilots() for (p = boss_xy) translate([p.x, p.y, base_t - 1]) cylinder(d = boss_pilot, h = 11);

module knob_mount() {
  top_in = H - wall + 0.5; // reach into the top wall
  // pads the board is pushed against, near its corners, clear of the EC11
  for (sx = [-1, 1], y = [ky_y0 + 2.5, ky_y0 + ky_l - 4])
    translate([knob_x + sx * (ky_w / 2 - 1.8), y, ky_face_z + eps]) cylinder(d = 3, h = top_in - ky_face_z);
  // snap hooks on the long edges, beside the shaft
  for (sx = [-1, 1]) {
    edge = knob_x + sx * (ky_w / 2 + 0.2);
    box([min(edge, edge + sx * 1.4), knob_y - 3, ky_back_z - 1.8], [max(edge, edge + sx * 1.4), knob_y + 3, top_in]);
    // barb: flat face under the board, ramp below for pushing it in
    hull() {
      box([min(edge, edge - sx * 1.0), knob_y - 3, ky_back_z - 0.9], [max(edge, edge - sx * 1.0), knob_y + 3, ky_back_z - 0.15]);
      box([min(edge, edge + sx * 0.5), knob_y - 3, ky_back_z - 1.8], [max(edge, edge + sx * 0.5), knob_y + 3, ky_back_z - 1.7]);
    }
  }
}

module vents() {
  // top, left and right of the knob, over the ESP32: its warm air leaves here
  for (xs = [[boss_in + 1, knob_x - ky_w / 2 - 3.5], [knob_x + ky_w / 2 + 3.5 - 2, W - boss_in - 1 - 2]])
    for (x = [xs[0]:4:xs[1]]) box([x, ky_y0 + 4, H - wall - 1], [x + 2, D - boss_d - 4, H + 1]);
  // back, low: intake, left and right of the USB
  for (xs = [[boss_d + 2, usb_x - 11], [usb_x + 9, W - boss_d - 2]])
    for (x = [xs[0]:4:xs[1]]) box([x, D - wall - 1, base_t + 3], [x + 2, D + 1, base_t + 12]);
}

module shell() difference() {
  union() {
    difference() {
      outer();
      cavity();
    }
    intersection() {
      outer();
      union() {
        bosses();
        tft_mount();
        knob_mount();
      }
    }
  }
  boss_pilots();
  tft_window();
  tft_pilots();
  vents();
  translate([usb_x, D - wall - 1, usb_z]) stadium(usb_hole[0], usb_hole[1], wall + 2);
  translate([knob_x, knob_y, H - wall - 1]) cylinder(d = knob_hole, h = wall + 2);
}

// ------------------------------------------------------------ base
perf_cx = (perf_x0 + perf_x1) / 2;
perf_cy = (perf_y0 + perf_y1) / 2;
perf_holes = [[perf_cx, perf_cy + perf_m3 / 2]];
perf_m2_holes = [for (sx = [-1, 1]) [perf_cx + sx * perf_m2[0] / 2, perf_cy - perf_m2[1] / 2]];
perf_rests = [for (sx = [-1, 1]) [perf_cx + sx * perf_m2[0] / 2, perf_cy + perf_m2[1] / 2]];

module base() {
  x0 = wall + clr;
  y0 = wall + clr;
  difference() {
    box([x0, y0, 0], [W - x0, D - y0, base_t]);
    for (p = boss_xy) translate([p.x, p.y, -1]) {
      cylinder(d = 3.4, h = base_t + 2);
      cylinder(d = 6.5, h = 1 + 2); // M3 head counterbore, 1 mm left
    }
    slots(12, W - 12, perf_y0 + 8, perf_y1 - 8, -1, base_t + 1); // intake under the ESP32
  }
  for (p = perf_holes) translate([p.x, p.y, base_t - eps]) difference() {
    cylinder(d = 6, h = perf_standoff);
    cylinder(d = 2.5, h = perf_standoff + 1); // M3 x 6 self-tapping, round head (the user's kit)
  }
  for (p = perf_m2_holes) translate([p.x, p.y, base_t - eps]) difference() {
    cylinder(d = 5, h = perf_standoff);
    cylinder(d = 1.6, h = perf_standoff + 1); // M2 x 6 self-tapping
  }
  for (p = perf_rests) translate([p.x, p.y, base_t - eps]) cylinder(d = 3.5, h = perf_standoff);
}

// ------------------------------------------------------------ stand-ins
module tft_standin(plug = true) tft_frame() {
  color("black") box([-tft_glass_w / 2, eps, tft_glass_z0], [tft_glass_w / 2, tft_front, tft_glass_z0 + tft_glass_h]);
  color("steelblue") difference() {
    box([-tft_w / 2, tft_front + eps, -tft_h / 2], [tft_w / 2, tft_front + tft_pcb_t, tft_h / 2]);
    box([-tft_notch_w / 2, 0, tft_h / 2 - tft_notch_d], [tft_notch_w / 2, 10, tft_h / 2 + 1]);
    for (h = tft_holes) translate([h.x, 0, h.y]) rotate([-90, 0, 0]) cylinder(d = tft_hole_d, h = 10);
  }
  // flat cable: out of the glass's top step, round the notch's edge, to its connector on the back
  nz = tft_h / 2 - tft_notch_d;
  fz = tft_h / 2 - tft_glass_top + tft_flex_bulge; // top of the bend
  back = tft_front + tft_pcb_t;
  color("orange") {
    box([-tft_flex_w / 2, 0.6, tft_glass_z0 + tft_glass_h - 0.5], [tft_flex_w / 2, tft_front, fz]);
    box([-tft_flex_w / 2, 0.6, nz + eps], [tft_flex_w / 2, back + 0.3, fz]);
    box([-tft_flex_w / 2 - 0.5, back + eps, nz - tft_flex_conn_l], [tft_flex_w / 2 + 0.5, back + tft_flex_conn, nz + 1]);
  }
  color("steelblue") box([-11, tft_front + tft_pcb_t, -tft_h / 2 + tft_hdr_edge + 3], [11, tft_front + tft_pcb_t + tft_back, 15]);
  hz = -tft_h / 2 + tft_hdr_edge;
  color("dimgray") box([-4 * p, tft_front + tft_pcb_t, hz - p / 2], [4 * p, tft_front + tft_pcb_t + tft_pins, hz + p / 2]);
  if (plug) color("gold", 0.35) box([-4 * p - 0.6, tft_front + tft_pcb_t + tft_pins, hz - 1.6], [4 * p + 0.6, tft_front + tft_pcb_t + tft_pins + tft_dupont, hz + 1.6]);
}

module esp_standin() translate([esp_x0, esp_y0, 0]) {
  color("dimgray") for (dx = [0, 10 * p])
    box([esp_row_edge + dx - p / 2, esp_pin_edge - p / 2, perf_z + perf_t], [esp_row_edge + dx + p / 2, esp_pin_edge + 14.5 * p, esp_z]);
  color("black") box([0, 0, esp_z], [esp_w, esp_l, esp_z + esp_t]);
  color("silver") box([5, esp_antenna + 1, esp_z + esp_t], [esp_w - 5, 25, esp_z + esp_top]);
  color("silver") box([esp_w / 2 - 4.45, esp_l - 7, esp_z + esp_t], [esp_w / 2 + 4.45, esp_l + 0.5, esp_z + esp_top]);
  color("orange", 0.5) box([0, 0, esp_z + esp_t], [esp_w, esp_antenna, esp_z + esp_top]); // antenna: keep clear
}

module knob_standin(plug = true) {
  color("firebrick") {
    box([knob_x - ky_w / 2, ky_y0, ky_back_z], [knob_x + ky_w / 2, ky_y0 + ky_l, ky_face_z]);
    translate([knob_x, knob_y, ky_face_z + ky_body_h / 2]) cube([ky_body, ky_body, ky_body_h], center = true);
  }
  color("dimgray") box([knob_x - 2.5 * p, ky_y0 + ky_l, ky_back_z], [knob_x + 2.5 * p, ky_y0 + ky_l + ky_pins, ky_face_z + p]);
  if (plug) color("gold", 0.35) box([knob_x - 2.5 * p - 0.6, ky_y0 + ky_l + ky_pins, ky_back_z - 0.8], [knob_x + 2.5 * p + 0.6, ky_y0 + ky_l + ky_pins + ky_dupont, ky_face_z + p + 0.8]);
  color("gray") translate([knob_x, knob_y, H + knob_gap]) cylinder(d = ky_cap_d, h = ky_cap_h);
}

module carrier_standin() color("darkgreen") box([perf_x0, perf_y0, perf_z], [perf_x1, perf_y1, perf_z + perf_t]);

module stand_ins() {
  tft_standin();
  carrier_standin();
  esp_standin();
  knob_standin();
}

// a name on a stick from point p, the text at dz above the case top,
// facing the front
module label(txt, p, dz) color("black") translate(p) {
  cylinder(d = 0.4, h = H + dz - p.z, $fn = 6);
  translate([0, 0, H + dz - p.z + 0.5]) rotate([90, 0, 0]) linear_extrude(0.4) text(txt, size = 2.5, halign = "center");
}

// panel coordinates (x, inwards, up the slope) to world
function panel_pt(x, y, z) = [W / 2 + x, y * cos(tilt) + z * sin(tilt), skirt_h + z * cos(tilt) - y * sin(tilt)];

module labels() {
  label("TFT", panel_pt(0, wall + tft_front + tft_pcb_t + tft_back, tft_v + 12), 12);
  label("ESP32", [usb_x, esp_y0 + 30, esp_z + esp_top], 3);
  label("antenna", [usb_x, esp_y0 + 3, esp_z + esp_top], 16);
  label("USB-C", [usb_x, esp_y1, esp_z + esp_top], 20);
  label("carrier", [perf_x1 - 4, perf_y0 + 2, perf_z + perf_t], 9);
  label("KY-040", [knob_x, knob_y, H + knob_gap + ky_cap_h], 18);
}

// ------------------------------------------------------------ output
// first print: the screen panel and the front of the top, with the knob mount
module test_front() intersection() {
  shell();
  box([-1, -1, skirt_h - 3], [W + 1, ky_y0 + ky_l + 3, H + 1]);
}

if (part == "shell") translate([0, D, H]) rotate([180, 0, 0]) shell();
else if (part == "test_front") translate([0, D, H]) rotate([180, 0, 0]) test_front();
else if (part == "base") base();
else if (part == "dims") echo(W = W, D = D, H = H, wall = wall, tilt = tilt, skirt_h = skirt_h, run = run,
  panel_len = panel_len, tft_v = tft_v, tft_w = tft_w, tft_h = tft_h, lit_z = lit_z, win_w = win_w, win_h = win_h,
  knob_x = knob_x, knob_y = knob_y, knob_hole = knob_hole, ky_y0 = ky_y0, ky_l = ky_l, ky_w = ky_w,
  usb_x = usb_x, usb_z = usb_z, usb_w = usb_hole[0], usb_h = usb_hole[1], boss_in = boss_in, boss_d = boss_d,
  perf_x0 = perf_x0, perf_y0 = perf_y0, perf_w = perf_w, perf_d = perf_d, perf_m3 = perf_m3, perf_m2_x = perf_m2[0], perf_m2_y = perf_m2[1], esp_x0 = esp_x0, esp_y0 = esp_y0,
  esp_w = esp_w, esp_l = esp_l, grid_x0 = grid_x0, grid_y0 = grid_y0, p = p); // for cardboard.py
else if (part == "clash") {
  // printed parts against every stand-in
  intersection() {
    union() {
      shell();
      base();
    }
    stand_ins();
  }
  // stand-ins against each other (plugs included)
  intersection() {
    tft_standin();
    union() {
      carrier_standin();
      esp_standin();
      knob_standin();
    }
  }
  intersection() {
    knob_standin();
    esp_standin();
  }
}
else {
  intersection() {
    union() {
      if (show_shell) color("white") shell();
      if (show_base) color("khaki") base();
    }
    if (cut >= 0) box([cut, -10, -10], [W + 10, D + 10, H + 10]);
    else box([-10, -10, -10], [W + 10, D + 10, H + 10]);
  }
  stand_ins();
  if (show_labels) labels();
}
