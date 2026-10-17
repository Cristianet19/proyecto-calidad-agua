#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_random.h"

static const char *TAG = "water";

// Struct with all the measurements of each sample
typedef struct {
    float tds;          // ppm
    float ph;
    float turbidity;    // NTU
    float temperature;  // °C
} reading_t;

static QueueHandle_t sensors_queue;

static void sensor_task(void *pvParameters){

    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        // Generation of simulated values (to be replaced by real sensor readings later)
        reading_t r = {
            .tds         = 150 + (esp_random() % 200),
            .ph          = 6.5f + (esp_random() % 20) / 10.0f,
            .turbidity   = (esp_random() % 50) / 10.0f,
            .temperature = 18.0f + (esp_random() % 80) / 10.0f,
        };
        ESP_LOGI(TAG, "Generated: TDS=%.0f pH=%.1f Turb=%.1f T=%.1f",
                 r.tds, r.ph, r.turbidity, r.temperature);

        // Values sended thru a queue and verification
        if (xQueueSend(sensors_queue, &r, 0) != pdTRUE) {
            ESP_LOGW(TAG, "Queue full, reading dropped");
        }

         vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(2000));
    } 
}

void app_main(void)
{
    sensors_queue = xQueueCreate(10, sizeof(reading_t));
    if (sensors_queue == NULL) {
        ESP_LOGE(TAG, "Could not create the queue");
        return;
    }

    xTaskCreate(sensor_task, "sensors", 4096, NULL, 5, NULL);
}