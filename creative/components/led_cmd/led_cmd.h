#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "uart_console.h"

#define LED1            27
#define LED2            26
#define LED3            33

#define LED_ON_HELP     "Usage: led on led_num"
#define LED_OFF_HELP    "Usage: led off led_num"

typedef enum {
    LED_ON = 0,
    LED_OFF,
} t_cmd;

void *led_cmd(void *arg);
