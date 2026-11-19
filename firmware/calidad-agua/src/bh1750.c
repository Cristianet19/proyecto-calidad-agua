#include "bh1750.h"
#include "i2c_bus.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BH1750_ADDR        0x23
#define BH1750_POWER_ON    0x01
#define BH1750_CONT_HRES   0x10   // continuous mode, 1 lx resolution
#define I2C_TIMEOUT_MS     100

static const char *TAG = "bh1750";
static i2c_master_dev_handle_t s_dev = NULL;

esp_err_t bh1750_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BH1750_ADDR,
        .scl_speed_hz = 100000,
    };
    esp_err_t err = i2c_master_bus_add_device(i2c_bus_get(), &cfg, &s_dev);
    if (err != ESP_OK) {
        return err;
    }

    uint8_t cmd = BH1750_POWER_ON;
    err = i2c_master_transmit(s_dev, &cmd, 1, I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }

    cmd = BH1750_CONT_HRES;
    err = i2c_master_transmit(s_dev, &cmd, 1, I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }

    // First conversion takes up to 180 ms
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "BH1750 ready");
    return ESP_OK;
}

esp_err_t bh1750_read(float *lux)
{
    uint8_t d[2];
    esp_err_t err = i2c_master_receive(s_dev, d, sizeof(d), I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }

    uint16_t raw = ((uint16_t)d[0] << 8) | d[1];
    *lux = raw / 1.2f;
    return ESP_OK;
}