#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"

// Creates the shared I2C bus (GPIO 21 = SDA, GPIO 22 = SCL). Call it once.
esp_err_t i2c_bus_init(void);

// Returns the shared bus handle, used by the sensor drivers.
i2c_master_bus_handle_t i2c_bus_get(void);

// Probes every address and logs the devices found (debug only).
void i2c_bus_scan(void);