#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "fb.h"

// SSD1315/SSD1306 128x64 OLED on I2C.

// Probes the bus for the display at 0x3C/0x3D, runs the init sequence.
esp_err_t display_init(i2c_master_bus_handle_t bus);

// Sends the pages of fb that differ from what was sent last (all of them
// the first time).
esp_err_t display_show(const fb_t *fb);
