#pragma once

#include "esp_err.h"

// Adds the AHT20 to the shared I2C bus and initializes it. Call after i2c_bus_init().
esp_err_t aht20_init(void);

// Triggers a measurement and returns temperature (°C) and relative humidity (%).
esp_err_t aht20_read(float *temperature, float *humidity);