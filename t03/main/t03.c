#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/timer.h"
#include "uart_console.h"
#include "sh1106.h"

#define ONE_SEC 4000 // true if timer_config_t.divider = 20000

static TaskHandle_t dclock_handler;
static QueueHandle_t set_time_queue;

void IRAM_ATTR timer_isr(void *param) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    timer_spinlock_take(TIMER_GROUP_0);
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
    vTaskNotifyGiveFromISR(dclock_handler, &xHigherPriorityTaskWoken);
    timer_spinlock_give(TIMER_GROUP_0);
}

static void digital_clock_task(void *display) {
    uint64_t time = 4567;
    char str[32] = {0};
    uint8_t hrs = 0;
    uint8_t min = 0;
    uint8_t sec = 0;

    while (true)  {
        if (ulTaskNotifyTake(pdFALSE, portMAX_DELAY) == pdTRUE) {
            if (xQueueReceive(set_time_queue, &time, 0) != pdPASS)
                time++;
            hrs = time / 3600;
            min = time % 3600 / 60;
            sec = time % 3600 % 60;
            sprintf(str, "%02d:%02d:%02d", hrs, min, sec);
            sh1106_str_in_display((sh1106_t*)display, str);
            sh1106_update((sh1106_t*)display);
        }
    }
}

static void *set_time(void *arg) {
    if (arg == NULL) {
        return NULL;
    }
    int len = 0;
    for (; ((char*)arg)[len] != '\0'; len++);
    if (len != 8) {
        uart_print_nl();
        uart_printstr("set_time usage: set_time xx:xx:xx");
        return NULL;
    }
    uint8_t hrs = atoi((char*)arg);
    uint8_t min = atoi((char*)arg + 3);
    uint8_t sec = atoi((char*)arg + 6);

    uint32_t time = hrs * 3600 + min * 60 + sec;
    xQueueSend(set_time_queue, &time, portMAX_DELAY);
    return NULL;
}

void app_main(void) {
    char *set_time_cmd = "set_time";
    uart_config_t uart_config = {
        .baud_rate  = 9600,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    timer_config_t timer_conf = {
        .alarm_en    = TIMER_ALARM_EN,
        .counter_en  = TIMER_PAUSE,
        .intr_type   = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .divider     = 20000,
    };
    sh1106_t display;

    display.addr = I2C_ADDR;
    display.port = I2C_PORT;
    OLED_power_on();
    init_i2c();
    sh1106_init(&display);
    sh1106_clear(&display);
    sh1106_update(&display);

    timer_init(TIMER_GROUP_0, TIMER_0, &timer_conf);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, ONE_SEC);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, timer_isr,
                       NULL, ESP_INTR_FLAG_IRAM, NULL);
    xTaskCreate(digital_clock_task, "digital_clock_task",
                2048, &display, 1, &dclock_handler);
    timer_start(TIMER_GROUP_0, TIMER_0);
    
    set_time_queue = xQueueCreate(1, sizeof(uint32_t));

    add_cmd(set_time_cmd, set_time);
    init_uart_console(&uart_config, 2);
    // set_time("a");
}

