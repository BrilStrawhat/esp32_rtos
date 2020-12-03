#include "led_cmd.h"

static const char *TAG = "led_cmd";
static uint8_t task_exist_flag = 0;

static QueueHandle_t led_queue_arr[3];
static TaskHandle_t led_handle_arr[3];

static void print_help(t_cmd cmd_num) {
    uart_print_nl();
    switch(cmd_num) {
        case LED_ON:
            uart_printstr(LED_ON_HELP);
            break;
        case LED_OFF:
            uart_printstr(LED_OFF_HELP);
            break;
        case LED_PULSE:
            uart_printstr(LED_PULSE_HELP);
            break;
    }
} 

static void delete_led_pulse_task(uint8_t led_num) {
    ESP_LOGI(TAG, "delete_led_pulse_task: led_num: %d", led_num);
    vQueueDelete(led_queue_arr[led_num]); 
    vTaskDelete(led_handle_arr[led_num]);
    task_exist_flag &= ~(1u << led_num);
    if (task_exist_flag == 8) {
        ledc_fade_func_uninstall();
    }
    ESP_LOGI(TAG, "deleted");
}

static void change_led_level(uint8_t led_num, uint8_t level) {
    ESP_LOGI(TAG, "change_led_level: %d %d", led_num, level);
    if (((task_exist_flag >> led_num) & 1u) == 1)
        delete_led_pulse_task(led_num);
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

static uint8_t get_led_pin(uint8_t led_num) {
    switch (led_num) {
        case 0:
            return LED1;
        case 1:
            return LED2;
        case 2:
            return LED3;
    }
    return 0xff;
}

static void led_pulse_task(void *param) {
    ledc_channel_config_t channel_conf;
    uint8_t led_pin = 0;
    uint32_t fade_time_ms = 1000;
    uint8_t led_num = *(uint8_t*)param;

    led_pin = get_led_pin(led_num);
    channel_conf.gpio_num = led_pin;
    channel_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    channel_conf.channel = led_num;
    channel_conf.intr_type = LEDC_INTR_FADE_END;
    channel_conf.timer_sel = LEDC_TIMER_0; 
    channel_conf.duty = DUTY;
    channel_conf.hpoint = 0; // idk, from example;
    ledc_channel_config(&channel_conf);
    while (true) {
        if (xQueueReceive(led_queue_arr[led_num], &fade_time_ms, 0) == pdPASS) {
            ESP_LOGI(TAG, "led_pulse: led_num: %d, fade_time: %d", led_num, fade_time_ms);
        }
        ledc_set_fade_with_time(channel_conf.speed_mode,
                channel_conf.channel, DUTY, fade_time_ms);
        ledc_fade_start(channel_conf.speed_mode,
                channel_conf.channel, LEDC_FADE_WAIT_DONE);
        ledc_set_fade_with_time(channel_conf.speed_mode,
                channel_conf.channel, 0, fade_time_ms);
        ledc_fade_start(channel_conf.speed_mode,
                channel_conf.channel, LEDC_FADE_WAIT_DONE);
    }
}

static void set_frequency(uint8_t led_num, uint32_t led_frequent) {
    if (((task_exist_flag >> led_num) & 1U) == 0) {
       led_queue_arr[led_num] = xQueueCreate(3, sizeof(uint32_t));
       xQueueSend(led_queue_arr[led_num], &led_frequent, 0);
       xTaskCreate(led_pulse_task, "led_pulse_task", 2048, &led_num, 1,
               &led_handle_arr[led_num]);
       task_exist_flag |= 1u << led_num;
       ESP_LOGI(TAG, "created led_pulse_task for %d led", led_num);
    }
    else if (((task_exist_flag >> led_num) & 1U) == 1) {
       xQueueSend(led_queue_arr[led_num], &led_frequent, 0);
       ESP_LOGI(TAG, "send in queue %d led_pulse_task for %d led", led_frequent, led_num);
    }
}

static void config_timer() {
    ledc_timer_config_t timer_conf;

    timer_conf.speed_mode = LEDC_LOW_SPEED_MODE;
    timer_conf.duty_resolution = LEDC_TIMER_8_BIT;
    timer_conf.timer_num = LEDC_TIMER_0;
    timer_conf.freq_hz = FREQ_HZ;
    timer_conf.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&timer_conf);
}

static void run_led_pulse(char *arg) {
    if (arg == NULL) {
        print_help(LED_PULSE);
        return;
    }
    uint8_t led_num;
    float led_frequent;

    if (*arg > '0' && *arg <= '3')
        led_num = *arg - '0';
    else {
        print_help(LED_PULSE);
        return;
    }
    if (arg + 1 && arg + 2) {
        led_frequent = atof(arg + 2);
        if (led_frequent <= 0 || led_frequent >= 2) {
            print_help(LED_PULSE);
            return;
        }
    }
    else {
        print_help(LED_PULSE);
        return;
    }
    if (((task_exist_flag >> 4) & 1u) == 0) {
        config_timer();
        ledc_fade_func_install(0);
        task_exist_flag |= 1u << 4;
    }
    led_frequent *= 1000; 
    set_frequency(led_num - 1, (uint32_t)led_frequent); // easier count from 0 then from 1
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
    else if (!strncmp("pulse ", (char*)arg, 6))
        run_led_pulse(arg + 6);
    return NULL;
}
