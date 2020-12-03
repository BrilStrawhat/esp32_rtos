#include "clock.h"
#include "dht11.h"
#include "driver/dac.h"

static TaskHandle_t dclock_handler;
static TaskHandle_t alarm_handler;
static TaskHandle_t sound_handler;
static QueueHandle_t set_time_queue;
static QueueHandle_t set_alarm_queue;
static uint8_t sound_enable = 0;

static void IRAM_ATTR timer_isr(void *param) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    timer_spinlock_take(TIMER_GROUP_0);
    timer_group_clr_intr_status_in_isr(TIMER_GROUP_0, TIMER_0);
    timer_group_enable_alarm_in_isr(TIMER_GROUP_0, TIMER_0);
    vTaskNotifyGiveFromISR(dclock_handler, &xHigherPriorityTaskWoken);
    timer_spinlock_give(TIMER_GROUP_0);
}

static void digital_clock_task(void *display) {
    uint32_t time = 56400;
    uint32_t alarm = 0xfffff;
    char str[32] = {0};
    uint8_t cursor = 0;
    uint16_t tmp_hmd = 0;
    uint8_t count = 5;

    while (true)  {
        if (ulTaskNotifyTake(pdFALSE, portMAX_DELAY) == pdTRUE) {
            if (xQueueReceive(set_time_queue, &time, 0) != pdPASS)
                time++;
            xQueueReceive(set_alarm_queue, &alarm, 0);
            if (time == alarm)
                vTaskResume(alarm_handler);
            if (count == 5) {
                if ((tmp_hmd = read_tmp_hmd()) != 0xffff) {
                    cursor = 26;
                    sprintf(str, "%dC %d%% RH", (tmp_hmd >> 8) & 0xff, tmp_hmd & 0xff);
                    sh1106_clear((sh1106_t*)display);
                    sh1106_str_in_display_font6x8((sh1106_t*)display, str, 5, &cursor);
                    count = 0;
                }
            }
            else
                count++;
            
            cursor = 16;
            sprintf(str, "%02d:%02d:%02d",
                    time / 3600, time % 3600 / 60, time % 3600 % 60);
            sh1106_str_in_display_font11x16((sh1106_t*)display, str, 1, &cursor);
            sh1106_update((sh1106_t*)display);
            if (time == 86400)
                time = 0;
        }
    }
}

static void alarm_task(void *display) {
    uint8_t count = 0;

    while (true) {
        if ((sound_enable & 1) == 1 && ((sound_enable >> 1) & 1) != 1) {
            sound_enable |= 1 << 1;
            xTaskNotifyGive(sound_handler);
            i2s_start(I2S_NUM);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS);
        sh1106_send_cmd((sh1106_t*)display, 0xA5);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        sh1106_send_cmd((sh1106_t*)display, 0xA4);
        if (count < 60)
            count++;
        else {
            if ((sound_enable & 1) == 1) {
                i2s_stop(I2S_NUM);
                sound_enable &= ~(1 << 1);
            }
            count = 0;
            vTaskSuspend(NULL);
        }
    }
}

void *set_time(void *arg) {
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
    if (time > 86400) {
        uart_print_nl();
        uart_printstr("set_time: out of range");
    }
    xQueueSend(set_time_queue, &time, portMAX_DELAY);
    return NULL;
}

void *set_alarm(void *arg) {
    if (arg == NULL) {
        return NULL;
    }
    uint32_t len = 0;
    for (; ((char*)arg)[len] != '\0'; len++);
    if (len != 8) {
        uart_print_nl();
        uart_printstr("set_time usage: set_time xx:xx:xx");
        return NULL;
    }
    uint8_t hrs = atoi((char*)arg);
    uint8_t min = atoi((char*)arg + 3);
    uint8_t sec = atoi((char*)arg + 6);

    uint32_t alarm = hrs * 3600 + min * 60 + sec;
    if (alarm > 86400) {
        uart_print_nl();
        uart_printstr("set_alarm: out of range");
    }
    xQueueSend(set_alarm_queue, &alarm, portMAX_DELAY);
    return NULL;
}

/* Will have stack overflow if run it not as the task */ 
void sound_task(void *param) {
    uint32_t src = 0xffffff;
    size_t bytes_written = 0;
    const i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
        .sample_rate = 44100,
        .bits_per_sample = 16, /* the DAC module will only take the 8bits from MSB */
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = 0, // default interrupt priority
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = 1
    };

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
    while (true) {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
            i2s_write(I2S_NUM, &src, sizeof(src), &bytes_written, 0);
        }
    }
}

void *sound(void *arg) {
    if (arg == NULL) {
        return NULL;
    }
    if (strcmp((char*)arg, "enable") == 0) {
        if ((sound_enable & 1) == 0) {
            sound_enable |= 1;
        }
    }
    else if (!strcmp((char*)arg, "disable")) {
        if ((sound_enable & 1) == 1) {
            sound_enable |= 0;
        }
    }
    else {
        uart_print_nl();
        uart_printstr("sound usage: sound enable/disable");
    }
    return NULL;
}

void run_clock(void *display) {
    timer_config_t timer_conf = {
        .alarm_en    = TIMER_ALARM_EN,
        .counter_en  = TIMER_PAUSE,
        .intr_type   = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = TIMER_AUTORELOAD_EN,
        .divider     = 20000,
    };

    timer_init(TIMER_GROUP_0, TIMER_0, &timer_conf);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, ONE_SEC);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, timer_isr,
                       NULL, ESP_INTR_FLAG_IRAM, NULL);
    xTaskCreatePinnedToCore(digital_clock_task, "digital_clock_task",
                2048, display, 10, &dclock_handler, 0);
    timer_start(TIMER_GROUP_0, TIMER_0);

    xTaskCreatePinnedToCore(alarm_task, "alarm_task",
                2048, display, 2, &alarm_handler, 1);
    vTaskSuspend(alarm_handler);

    xTaskCreatePinnedToCore(sound_task, "sound_task",
                2048, NULL, 3, &sound_handler, 1);
    
    set_time_queue = xQueueCreate(1, sizeof(uint32_t));
    set_alarm_queue = xQueueCreate(1, sizeof(uint32_t));
}
