# Wiring

Each module goes straight to the ESP32, with no daisy-chaining. Pins as in
`main/display.c` and `main/encoder.c`. The board is the ESP32 mini
(D1 mini layout, CH9102F). Its pads: "inner" = the D1-mini-compatible
row, "outer" = the edge row. The pins were picked so each cable ends in
few Dupont housings (see "Housings on the mini" below); the 30-pin DevKit
on the breadboard is retired and its old wiring (knob 25/26/27, BLK D4)
no longer matches the firmware.

## TFT: ST7789V2 1.69" 240x280

| TFT pin | ESP32 pin    | Mini pad | Note                            |
|---------|--------------|----------|---------------------------------|
| GND     | GND          | right inner |                                 |
| VCC     | 3V3          | left inner | never VIN: the panel is 3.3 V only |
| SCL     | D18          | left inner | SPI clock                       |
| SDA     | D23          | left inner | SPI data (MOSI)                 |
| RES     | GPIO17 (TX2) | right inner | reset                           |
| DC      | GPIO16 (RX2) | right inner | data / command                  |
| CS      | D5           | left inner | chip select                     |
| BLK     | D19          | left inner | backlight, PWM dimming          |

## Knob: KY-040

| KY-040 pin | ESP32 pin | Mini pad | Note                    |
|------------|-----------|----------|-------------------------|
| +          | 3V3       | left inner |                         |
| GND        | GND       | right outer |                         |
| CLK        | D27       | right outer | channel A               |
| DT         | D25       | right outer | channel B               |
| SW         | D32       | right outer | push button, active low |

## Housings on the mini

Pads in each housing are next to each other, listed from the antenna end
towards the USB end. The board's silkscreen names two pads per line: the
outer one, then the inner one (`GND TXD`, `IO27 RXD`, ...).

| Housing | Row | Pads → module pin |
|---|---|---|
| 5-pin | left inner | IO18 → TFT SCL, IO19 → TFT BLK, IO23 → TFT SDA, IO5 → TFT CS, 3V3 → TFT VCC + knob + |
| 3-pin | right inner | IO17 → TFT RES, IO16 → TFT DC, GND → TFT GND |
| 4-pin | right outer | GND → knob GND, IO27 → CLK, IO25 → DT, IO32 → SW |

- TFT signals only, leaving out power (3V3 and GND are the last pad of
  each TFT housing above):

  | Housing | Row | Pads → TFT pin |
  |---|---|---|
  | 4-pin | left inner | IO18 → SCL, IO19 → BLK, IO23 → SDA, IO5 → CS |
  | 2-pin | right inner | IO17 → RES, IO16 → DC |
- 3V3 has one pad: the TFT's VCC and the knob's + are two wires crimped
  into one terminal (twist the stripped ends together, crimp as one wire).
- The pad after GND on the right inner row is 5V (labelled VCC): a 3-pin
  housing shifted one pad down puts 5 V on the TFT.
- The inner pads beside the knob's housing are TXD / RXD (USB serial):
  leave them empty.
- Cable lengths (estimates from the case size, not measured in the model):
  TFT 12 cm, knob 15 cm, so the base can lie beside the shell with
  everything plugged in. 26 AWG stranded.

No row has six usable pads in a row, so the TFT can't be one housing. An
all-in-order TFT layout exists (left inner 3V3, 5 = SCL, 23 = SDA, 19 = RES,
18 = DC, 26 = CS, plus right inner 16 = BLK, GND) but takes the clock off
its IOMUX pin (18), so SPI goes through the GPIO matrix: not tried, since
the layout above keeps the pins proven at 40 MHz.

## Pins left free

- D21 / D22 (I2C): free since the OLED was dropped.
- D4, D26: free since the knob and BLK moved (2026-10-01).
- D2: not used, it drives the onboard LED.
- Not usable: GPIO6-11 (the module's flash; seller diagrams show 9 and 10
  as free, they aren't), GPIO1/3 (USB serial), 34-39 are input-only,
  0/2/12/15 are read at boot.
