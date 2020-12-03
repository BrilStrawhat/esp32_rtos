#include "dht11.h"

static esp_err_t dht_send_start_sign() {
    gpio_set_direction(DHT_DATA, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_DATA, 0);
    ets_delay_us(18000);
    gpio_set_level(DHT_DATA, 1);
    ets_delay_us(40);
    gpio_set_direction(DHT_DATA, GPIO_MODE_INPUT);
    return ESP_OK;
}

static bool check_response() {
    int count = 0;

    while (gpio_get_level(DHT_DATA) == 0) {
        count++;
        ets_delay_us(1);
        if (count > 80) {
            printf("To long 0 signal from DHT11\n");
            return 1;
        }
    }
    count = 0;
    while (gpio_get_level(DHT_DATA) == 1) {
        count++;
        ets_delay_us(1);
        if (count > 80) {
            printf("To long 1 signal from DHT11\n");
            return 1;
        }
    }
    return 0;
}

/*
    time: time of signal;
    bit: expected signal
*/
static int read_bit() {
    int counter = 0;

    while (gpio_get_level(DHT_DATA) == 0) {
        ets_delay_us(1);
        counter++;
        if (counter > 50) {
            printf("Failed to read bit from DHT11");
            return -1;
        }
    }
    counter = 0;
    while (gpio_get_level(DHT_DATA) == 1) {
        ets_delay_us(1);
        counter++;
        if (counter > 70) {
            printf("Failed to read bit from DHT11");
            return -1;
        }
    }
    if (counter <= 28)
        return 0;
    else
        return 1;
}

esp_err_t dht_init() {
    esp_err_t rc = ESP_OK;

    if ((rc = gpio_set_direction(DHT_POW, GPIO_MODE_OUTPUT)) != ESP_OK) {
        printf("line %d: error code: %d\n", (__LINE__ - 1), rc);
        return rc;
    }
    if ((rc = gpio_set_level(DHT_POW, 1)) != ESP_OK) {
        printf("line %d: error code: %d\n", (__LINE__ - 1), rc);
        return rc;
    }
    ets_delay_us(2000000);
    return rc;
}

/*
* (dht_res >> 8) & 0xff get temperature
* dht_res & 0xff get humidity 
*/
uint16_t read_tmp_hmd() {
    uint8_t data[5] = {0};
    uint16_t result = 0;

    dht_send_start_sign();
    for (uint8_t i = 0; check_response() != 0; i++) {
        dht_send_start_sign();
        if (i > 2) 
            return 0xffff;
    }
    for (uint8_t i = 0; i < 5; i++) {
       for (uint8_t j = 0; j < 8; j++) {
           data[i] <<= 1;
           data[i] += read_bit();
       }
    }
    if (data[0] + data[1] + data[2] + data[3] != data[4]) {
        printf("dht11 byte sum is incorrect, the result could be incorrect");
        return 0xffff;
    }
    result = data[2];
    result = result << 8;
    result += data[0]; 
    return result;
}
