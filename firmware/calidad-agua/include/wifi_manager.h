#pragma once

#include <stdbool.h>
#include "freertos/FreeRTOS.h"

// Starts WiFi in station mode and connects in the background
void wifi_manager_init(void);

// Blocks until WiFi is connected or the timeout expires
bool wifi_manager_wait_connected(TickType_t timeout_ticks);