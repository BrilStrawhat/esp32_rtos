#include "led_cmd.h"

static const char *TAG = "led_cmd";

static void change_led_level(uint8_t led_num, uint8_t level) {
    ESP_LOGI(TAG, "change_led_level: %d %d", led_num, level);

    switch(led_num) {
        case 0:
            gpio_set_direction(LED1, GPIO_MODE_OUTPUT);
            gpio_set_level(LED1, level);
            break;
        case 1:
            gpio_set_direction(LED2, GPIO_MODE_OUTPUT);
            gpio_set_level(LED2, level);
            break;
        case 2:
            gpio_set_direction(LED3, GPIO_MODE_OUTPUT);
            gpio_set_level(LED3, level);
            break;
    }
}

static void switch_led(char *arg, uint8_t level) {
    if (arg == NULL)
        return;
    while (arg) {
        ESP_LOGI(TAG, "[line num:%d] %s", __LINE__, arg);
        if (*arg < '1' || *arg > '3')
            return;
        change_led_level(*arg - '0' - 1, level); // convert to int and minus one for counting from 0 not from 1
        if (*(arg + 1) == ' ')
            arg += 2;
        else
            arg++;
    }
}

void *led_cmd(void *arg) {
    esp_log_level_set(TAG, ESP_LOG_NONE);
    if (arg == NULL) {
        ESP_LOGE(TAG, "command is NULL");
        return NULL;
    }
    ESP_LOGI(TAG, "%s", (char*)arg);
    if (!strncmp("on ", (char*)arg, 3))
        switch_led(arg + 3, 1);
    else if (!strncmp("off ", (char*)arg, 4))
        switch_led(arg + 4, 0);
    return NULL;
}

