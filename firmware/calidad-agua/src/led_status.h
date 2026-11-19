#pragma once
 
// RGB status LED (common anode) used to show the water quality state.
// Pins: R = GPIO 19, G = GPIO 18, B = GPIO 23.
 
typedef enum {
    LED_STATUS_CLEAN = 0,     // green
    LED_STATUS_DOUBTFUL,      // yellow
    LED_STATUS_CONTAMINATED,  // red
    LED_STATUS_OFF            // all off
} led_status_t;
 
// Configure the LED pins and leave the LED off.
void led_status_init(void);
 
// Show the given state on the LED.
void led_status_show(led_status_t status);
 