#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "uart_console.h"
#include "sh1106.h"
#include "clock.h"
#include "dht11.h"
#include "led_cmd.h"

void app_main(void) {
    uart_config_t uart_config = {
        .baud_rate  = 9600,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    sh1106_t display;

    display.addr = I2C_ADDR;
    display.port = I2C_PORT;
    OLED_power_on();
    init_i2c();
    sh1106_init(&display);
    sh1106_clear(&display);
    sh1106_update(&display);

    dht_init();
    run_clock(&display);

    add_cmd("set_time", set_time);
    add_cmd("set_alarm", set_alarm);
    add_cmd("sound", sound);
    add_cmd("led", led_cmd);
    init_uart_console(&uart_config, 1);
}

