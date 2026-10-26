#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_random.h"
#include "wifi_manager.h"

static const char *TAG = "water";

// Struct with all the measurements of each sample
typedef struct {
    float tds;          // ppm
    float ph;
    float turbidity;    // NTU
    float temperature;  // °C
} reading_t;

// Enum with the possible states of water quality
typedef enum {
    WATER_CLEAN,
    WATER_DOUBTFUL,
    WATER_CONTAMINATED
} water_state_t;

// Message sent from processing_task to mqtt_task
typedef struct {
    reading_t reading;
    water_state_t state;
} water_msg_t;

static QueueHandle_t sensors_queue;
static QueueHandle_t mqtt_queue;
 
// Provisional rule-based classifier.
// Later this function will be replaced by the Edge Impulse model.
static water_state_t classify(const reading_t *r)
{
    if (r->tds >= 600 || r->ph < 6.0f || r->ph > 9.0f || r->turbidity >= 4.0f) {
        return WATER_CONTAMINATED;
    }
    if (r->tds >= 300 || r->ph < 6.5f || r->ph > 8.5f || r->turbidity >= 1.0f) {
        return WATER_DOUBTFUL;
    }
    return WATER_CLEAN;
}
 
static const char *state_to_string(water_state_t s)
{
    switch (s) {
    case WATER_CLEAN:        return "CLEAN";
    case WATER_DOUBTFUL:     return "DOUBTFUL";
    case WATER_CONTAMINATED: return "CONTAMINATED";
    default:                 return "UNKNOWN";
    }
}
 
static void sensor_task(void *pvParameters)
{
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
 
        // Values sent through a queue and verification
        if (xQueueSend(sensors_queue, &r, 0) != pdTRUE) {
            ESP_LOGW(TAG, "Queue full, reading dropped");
        }
 
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(2000));
    }
}
 
static void processing_task(void *pvParameters)
{
    reading_t r;
 
    while (1) {
        // Blocks here until a reading arrives (no CPU used while waiting)
        if (xQueueReceive(sensors_queue, &r, portMAX_DELAY) == pdTRUE) {
            water_state_t state = classify(&r);
            ESP_LOGI(TAG, "Processed: TDS=%.0f pH=%.1f Turb=%.1f -> %s",
                     r.tds, r.ph, r.turbidity, state_to_string(state));
 
            // Non-blocking send: if the network side is slow, drop the message
            // instead of blocking the processing
            water_msg_t msg = { .reading = r, .state = state };
            if (xQueueSend(mqtt_queue, &msg, 0) != pdTRUE) {
                ESP_LOGW(TAG, "mqtt_queue full, message dropped");
            }
        }
    }
}

static void mqtt_task(void *pvParameters)
{
    // Wait for WiFi before consuming messages.
    // Meanwhile, mqtt_queue keeps buffering the latest readings
    while (!wifi_manager_wait_connected(pdMS_TO_TICKS(10000))) {
        ESP_LOGW(TAG, "mqtt_task: waiting for WiFi...");
    }
    ESP_LOGI(TAG, "mqtt_task: WiFi connected, start consuming messages");

    water_msg_t msg;

    while (1) {
        // Blocks until processing_task sends a message
        if (xQueueReceive(mqtt_queue, &msg, portMAX_DELAY) == pdTRUE) {
            // For now we only log what we will publish later
            ESP_LOGI(TAG, "MQTT (simulated): tds=%.0f ph=%.1f turb=%.1f temp=%.1f state=%s",
                     msg.reading.tds, msg.reading.ph, msg.reading.turbidity,
                     msg.reading.temperature, state_to_string(msg.state));
        }
    }
}
 
void app_main(void)
{
    sensors_queue = xQueueCreate(10, sizeof(reading_t));
    if (sensors_queue == NULL) {
        ESP_LOGE(TAG, "Could not create the queue");
        return;
    }
 
    mqtt_queue = xQueueCreate(10, sizeof(water_msg_t));
    if (mqtt_queue == NULL) {
        ESP_LOGE(TAG, "Could not create the mqtt queue");
        return;
    }
 
    xTaskCreate(sensor_task,     "sensors",    4096, NULL, 5, NULL);
    xTaskCreate(processing_task, "processing", 4096, NULL, 4, NULL);
    xTaskCreate(mqtt_task, "mqtt", 4096, NULL, 3, NULL);
 
    // WiFi starts after the measurement tasks, so sensing never depends on the network
    wifi_manager_init();
}
