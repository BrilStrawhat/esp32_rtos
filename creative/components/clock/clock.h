#include "sh1106.h"
#include "driver/timer.h"
#include "freertos/task.h"
#include "uart_console.h"
#include "driver/i2s.h"

#define ONE_SEC 4000 // true if timer_config_t.divider = 20000
#define I2S_NUM 0

void *set_time(void *arg);

void *set_alarm(void *arg);

void run_clock(void *display);

void *sound(void *arg);
