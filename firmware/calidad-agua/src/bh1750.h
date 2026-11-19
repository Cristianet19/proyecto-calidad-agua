#pragma once

#include "esp_err.h"

// Adds the BH1750 to the shared I2C bus and starts continuous measurement.
esp_err_t bh1750_init(void);

// Returns the last measured illuminance in lux.
esp_err_t bh1750_read(float *lux);