#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "driver/gpio.h"
#include "esp32/rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <unistd.h>

#define DHT_POW                 2
#define DHT_DATA                4

esp_err_t dht_init();
uint16_t read_tmp_hmd();
