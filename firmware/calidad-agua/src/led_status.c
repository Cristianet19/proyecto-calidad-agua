#include <stdbool.h>
#include "driver/gpio.h"
#include "led_status.h"
 
#define LED_RED     GPIO_NUM_19
#define LED_GREEN   GPIO_NUM_18
#define LED_BLUE    GPIO_NUM_23
 
// Common anode: the LED turns on with a LOW level
#define LED_ON      0
#define LED_OFF     1
 
static void led_set_rgb(bool red, bool green, bool blue) {
    gpio_set_level(LED_RED,   red   ? LED_ON : LED_OFF);
    gpio_set_level(LED_GREEN, green ? LED_ON : LED_OFF);
    gpio_set_level(LED_BLUE,  blue  ? LED_ON : LED_OFF);
}
 
void led_status_init(void) {
    const gpio_num_t pins[] = {LED_RED, LED_GREEN, LED_BLUE};
    for (int i = 0; i < 3; i++) {
        gpio_reset_pin(pins[i]);
        gpio_set_direction(pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(pins[i], LED_OFF);
    }
}
 
void led_status_show(led_status_t status) {
    switch (status) {
        case LED_STATUS_CLEAN:        led_set_rgb(false, true,  false); break; // green
        case LED_STATUS_DOUBTFUL:     led_set_rgb(true,  true,  false); break; // yellow
        case LED_STATUS_CONTAMINATED: led_set_rgb(true,  false, false); break; // red
        default:                      led_set_rgb(false, false, false); break; // off
    }
}