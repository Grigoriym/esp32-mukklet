# Wiring

Each module goes straight to the ESP32, with no daisy-chaining. Pins as in
`main/display.c` and `main/encoder.c`. The breadboard uses the
ESP32-WROOM-32 DevKit (30-pin); the enclosure will use the ESP32 mini
(D1 mini layout, CH9102F), same GPIOs. Its pads: "inner" = the
D1-mini-compatible row, "outer" = the edge row.

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
| BLK     | D4           | right outer | backlight, PWM dimming          |

## Knob: KY-040

| KY-040 pin | ESP32 pin | Mini pad | Note                    |
|------------|-----------|----------|-------------------------|
| +          | 3V3       | left inner |                         |
| GND        | GND       | right outer |                         |
| CLK        | D25       | right outer | channel A               |
| DT         | D26       | left inner | channel B               |
| SW         | D27       | right outer | push button, active low |

## Pins left free

- D21 / D22 (I2C): free since the OLED was dropped.
- D2: not used, it drives the onboard LED.

On the mini, 3V3 has one pad: both 3V3 wires (TFT VCC, knob +) share it.
