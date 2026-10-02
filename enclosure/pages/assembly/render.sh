#!/usr/bin/env bash
# Re-renders the assembly page's pictures (s1..s8.png) from the case model.
# Run after the model changes, then check the labels in assembly.html still
# point at the right things (they are SVG in a 1000 x 750 box over each one).
set -euo pipefail
cd "$(dirname "$0")"
r() { # name, openscad args...
    local n=$1
    shift
    echo "$n.png"
    openscad -D show_labels=false --colorscheme=Tomorrow --imgsize=1000,750 "$@" -o "$n.png" ../../enclosure.scad >/dev/null 2>&1
}
only_base=(-D show_body=false -D show_front=false -D show_tft=false -D show_knob=false)
r s1 -D explode=45 -D explode_front=30 --camera=25,41,50,65,0,320,360
r s2 -D show_body=false -D show_base=false -D show_mini=false -D show_knob=false --camera=25,12,37,70,0,150,190
r s3 -D show_front=false -D show_base=false -D show_mini=false -D show_tft=false --camera=25,35,40,115,0,335,280
r s4 -D explode_front=28 -D show_base=false -D show_mini=false --camera=25,25,36,70,0,325,330
r s5 "${only_base[@]}" -D mini_tilt=8 -D mini_back=6 --camera=25,50,8,60,0,140,190
r s6 "${only_base[@]}" --camera=25,50,8,60,0,140,190
r s7 --camera=25,41,34,65,0,150,290
r s8 -D cut=25 --projection=o --camera=25,41,34,90,0,270,230
