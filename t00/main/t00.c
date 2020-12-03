#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/i2c.h"
#include "sh1106.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define I2C_SDA         21
#define I2C_SCL         22
#define EN_OLED         32
#define I2C_ADDR        SH1106_DEFAULT_ADDR
#define I2C_PORT        SH1106_DEFAULT_PORT
#define LDR_GPIO        ADC1_CHANNEL_0 // LDR - Light Decreasing Resistance

static const adc_bits_width_t width = ADC_WIDTH_BIT_9;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static QueueHandle_t brightness_queue;

void init_i2c() {
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA,
        .scl_io_num = I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 1000000
    };
    i2c_param_config(I2C_PORT, &i2c_config);
    i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0);
}

void adc_read_task(void *param) {
    uint32_t adc_reading = 0; 

    while(true) {
        adc_reading = adc1_get_raw(LDR_GPIO);
        adc_reading /= 2;
        adc_reading = ~adc_reading;
        xQueueSend(brightness_queue, &adc_reading, portMAX_DELAY);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}

void change_brightness_task(void *display) {
    uint32_t adc_reading = 0; 

    while (true) {
        if (xQueueReceive(brightness_queue, &adc_reading, portMAX_DELAY)) {
            sh1106_send_cmd(display, 0x81, &adc_reading);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

void app_main(void) {
    sh1106_t display;

    display.addr = I2C_ADDR;
    display.port = I2C_PORT;
    OLED_power_on(EN_OLED);
    init_i2c();
    sh1106_init(&display);

    adc1_config_width(width);
    adc1_config_channel_atten(LDR_GPIO, atten);

    sh1106_fill(&display);
    sh1106_update(&display);

    brightness_queue = xQueueCreate(2, sizeof(uint32_t));
    xTaskCreatePinnedToCore(change_brightness_task, "change_brightness", 2048u, &display, 1, 0, 1);
    xTaskCreatePinnedToCore(adc_read_task, "adc_read_task", 2048u, NULL, 1, 0, 1);
}
