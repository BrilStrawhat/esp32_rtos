#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/i2s.h"
#include "uart_console.h"

#define I2S_NUM 0

void *sound(void *arg) {
    if (arg == NULL) {
        return NULL;
    }
    if (strcmp((char*)arg, "enable") == 0) {
        printf("%s\n", (char*)arg);
        i2s_start(I2S_NUM);
    }
    else if (!strcmp((char*)arg, "disable")) {
        printf("%s\n", (char*)arg);
        i2s_stop(I2S_NUM);
    }
    else {
        uart_print_nl();
        uart_printstr("sound usage: sound enable/disable");
    }
    return NULL;
}

void app_main(void) {
    char *sound_cmd = "sound";
    uint32_t src = 5555;
    size_t bytes_written = 0;

    uart_config_t uart_config = {
        .baud_rate  = 9600,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
        .sample_rate = 44100,
        .bits_per_sample = 16, /* the DAC module will only take the 8bits from MSB */
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0, // default interrupt priority
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
    i2s_set_sample_rates(I2S_NUM, 22050);
    i2s_write(I2S_NUM, &src, sizeof(src), &bytes_written, portMAX_DELAY);
    i2s_stop(I2S_NUM);
    add_cmd(sound_cmd, sound);
    init_uart_console(&uart_config, 2);
}

