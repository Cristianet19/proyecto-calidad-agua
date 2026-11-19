#include "aht20.h"
#include "i2c_bus.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define AHT20_ADDR      0x38
#define I2C_TIMEOUT_MS  100

static const char *TAG = "aht20";
static i2c_master_dev_handle_t s_dev = NULL;

esp_err_t aht20_init(void)
{
    i2c_device_config_t cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = AHT20_ADDR,
        .scl_speed_hz = 100000,
    };
    esp_err_t err = i2c_master_bus_add_device(i2c_bus_get(), &cfg, &s_dev);
    if (err != ESP_OK) {
        return err;
    }

    // The sensor needs about 40 ms after power-up
    vTaskDelay(pdMS_TO_TICKS(100));

    // Check the calibration bit (0x08) in the status byte
    uint8_t status_cmd = 0x71;
    uint8_t status = 0;
    err = i2c_master_transmit_receive(s_dev, &status_cmd, 1, &status, 1, I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }

    if ((status & 0x08) == 0) {
        uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};
        err = i2c_master_transmit(s_dev, init_cmd, sizeof(init_cmd), I2C_TIMEOUT_MS);
        if (err != ESP_OK) {
            return err;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    ESP_LOGI(TAG, "AHT20 ready (status=0x%02X)", status);
    return ESP_OK;
}

esp_err_t aht20_read(float *temperature, float *humidity)
{
    // Trigger a measurement
    uint8_t trigger[3] = {0xAC, 0x33, 0x00};
    esp_err_t err = i2c_master_transmit(s_dev, trigger, sizeof(trigger), I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }

    // The measurement takes about 80 ms
    vTaskDelay(pdMS_TO_TICKS(100));

    // Response: status, 20 bits humidity, 20 bits temperature, CRC
    uint8_t d[7];
    err = i2c_master_receive(s_dev, d, sizeof(d), I2C_TIMEOUT_MS);
    if (err != ESP_OK) {
        return err;
    }

    if (d[0] & 0x80) {
        return ESP_ERR_INVALID_STATE;   // still busy
    }

    uint32_t raw_h = ((uint32_t)d[1] << 12) | ((uint32_t)d[2] << 4) | (d[3] >> 4);
    uint32_t raw_t = ((uint32_t)(d[3] & 0x0F) << 16) | ((uint32_t)d[4] << 8) | d[5];

    *humidity = raw_h * 100.0f / 1048576.0f;
    *temperature = raw_t * 200.0f / 1048576.0f - 50.0f;
    return ESP_OK;
}