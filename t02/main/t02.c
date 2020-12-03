#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "uart_console.h"
#include "dht11.c"

typedef struct {
    uint32_t timestemp;
    uint16_t tmp_hmd;
} t_log_line;

static TaskHandle_t th_handle;
static t_log_line logs_arr[60];

static void print_help() {
    uart_print_nl();
    uart_printstr("Help output");
    return;
}

static void print_log() {
    char str[32] = {0};

    uart_print_nl();
    for (int8_t i = 59; i > -1; i--) {
        if (logs_arr[i].timestemp == 0 && logs_arr[i].tmp_hmd == 0) {
            continue;
        }
        if (logs_arr[i].timestemp != 0) {
            sprintf(str, "%dC %dRH %d", (logs_arr[i].tmp_hmd >> 8) & 0xff,
                                         logs_arr[i].tmp_hmd & 0xff,
                                         logs_arr[i].timestemp);
            uart_printstr(str);
        }
        else {
            sprintf(str, "%dC %dRH", (logs_arr[i].tmp_hmd >> 8) & 0xff,
                                      logs_arr[i].tmp_hmd & 0xff);
            uart_printstr(str);
        }
        if(i != 0)
            uart_print_nl();
    }
}

static void *get_th_log(void *arg) {
    if (arg == NULL) {
        return NULL;
    }
    if (!strncmp("get_th_log", (char*)arg, 10)) {
        if (*(char*)(arg + 10) != '\0') {
            print_help();
            return NULL;
        }
        vTaskSuspend(th_handle);
        print_log();
        vTaskResume(th_handle);
    }
    else
        print_help();
    return NULL;
}

static void measure_th_task(void *param) {
    t_log_line log_line;
    uint16_t old_tmp_hmd = 0xffff;
    uint32_t old_timestamp = 0;
    const uint16_t shift_count = sizeof(logs_arr) - sizeof(t_log_line);

    memset(logs_arr, 0, sizeof(logs_arr));
    while (true) {
        log_line.tmp_hmd = read_tmp_hmd();
        if (log_line.tmp_hmd == 0xffff) {
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }
        memmove(logs_arr + 1, logs_arr, shift_count);
        if (old_tmp_hmd != log_line.tmp_hmd) {
            log_line.timestemp = xTaskGetTickCount() / 100 - old_timestamp;
            old_timestamp = xTaskGetTickCount() / 100;
            old_tmp_hmd = log_line.tmp_hmd;
            logs_arr[0] = log_line;
        }
        else {
            log_line.timestemp = 0;
            logs_arr[0] = log_line;
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    char *get_th_log_cmd_name = "get_th_log";
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    dht_init();
    xTaskCreate(measure_th_task, "measure_th_task", 2048, NULL, 1, &th_handle);
    add_cmd(get_th_log_cmd_name, get_th_log);
    init_uart_console(&uart_config, 2);
}
