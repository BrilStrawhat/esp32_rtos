#include "sh1106.h"

#if FONT_SIZE == TWO_FONT 
#include "font6x8.h"
#include "font11x16.h"

static void sh1106_put_char_in_disply_font6x8(char chr, sh1106_t *display,
                                              uint8_t page, uint8_t *cursor) {
    if (chr < 32 || chr > 126) {
        printf("Invalid char in string for OLED: '%c' ASCII code: %d\n", chr, chr);
        exit(-1);
    }
    for (int i = 0; i < 6; i++) {
        display->pages[page][(*cursor)++] = font6x8[(chr - 32) * 6 + i];
    }
}

void sh1106_str_in_display_font6x8(sh1106_t *display, char *str,
                                   uint8_t page, uint8_t *cursor) {
    for (int i = 0; str[i] != '\0'; i++, (*cursor)++) {
        if (*cursor >= 122) {
            *cursor = 0;
            page++;
            if (page == 7) {
                printf("Too long str to display it on OLED, max size of str is 142");
                exit(-1);
            }
        }
        sh1106_put_char_in_disply_font6x8(str[i], display, page, cursor);
    }
} 

static void sh1106_put_char_in_disply_font11x16(char chr, sh1106_t *display,
                                                uint8_t page, uint8_t *cursor) {
    if (chr < 32 || chr > 126) {
        printf("Invalid char in string for OLED: '%c' ASCII code: %d\n", chr, chr);
        exit(-1);
    }
    for (int i = 0, k = *cursor; i < 22; i++, k++) {
        if (k == *cursor + 11) {
            page++;
            k = *cursor;
            *cursor += 11;
        }
        display->pages[page][k] = font11x16[(chr - 32) * 22 + i];
    }
}

void sh1106_str_in_display_font11x16(sh1106_t *display, char *str,
                                     uint8_t page, uint8_t *cursor) {
    for (int i = 0; str[i] != '\0'; i++, (*cursor)++) {
        if (*cursor >= 119) {
            *cursor = 0;
            page += 2;
            if (page == 7) {
                printf("Too long str to display it on OLED, max size of str is 142");
                exit(-1);
            }
        }
        sh1106_put_char_in_disply_font11x16(str[i], display, page, cursor);
    }
} 
#elif FONT_SIZE == FONT6x8 
#include "font6x8.h"

static void sh1106_put_char_in_disply(char chr, sh1106_t *display,
                               uint8_t page, uint8_t *cursor) {
    if (chr < 32 || chr > 126) {
        printf("Invalid char in string for OLED: '%c' ASCII code: %d\n", chr, chr);
        exit(-1);
    }
    for (int i = 0; i < 6; i++) {
        display->pages[page][(*cursor)++] = font6x8[(chr - 32) * 6 + i];
    }
}

void sh1106_str_in_display(sh1106_t *display, char *str) {
    uint8_t page = 0;
    uint8_t cursor = 0; 

    for (int i = 0; str[i] != '\0'; i++, cursor++) {
        if (cursor >= 122) {
            cursor = 0;
            page++;
            if (page == 7) {
                printf("Too long str to display it on OLED, max size of str is 142");
                exit(-1);
            }
        }
        sh1106_put_char_in_disply(str[i], display, page, &cursor);
    }
} 

#elif FONT_SIZE == FONT11x16
#include "font11x16.h"

static void sh1106_put_char_in_disply(char chr, sh1106_t *display,
                               uint8_t page, uint8_t *cursor) {
    if (chr < 32 || chr > 126) {
        printf("Invalid char in string for OLED: '%c' ASCII code: %d\n", chr, chr);
        exit(-1);
    }
    for (int i = 0, k = *cursor; i < 22; i++, k++) {
        if (k == *cursor + 11) {
            page++;
            k = *cursor;
            *cursor += 11;
        }
        display->pages[page][k] = font11x16[(chr - 32) * 22 + i];
    }
}

void sh1106_str_in_display(sh1106_t *display, char *str) {
    uint8_t page = 0;
    uint8_t cursor = 0; 

    for (int i = 0; str[i] != '\0'; i++, cursor++) {
        if (cursor >= 119) {
            cursor = 0;
            page += 2;
            if (page == 7) {
                printf("Too long str to display it on OLED, max size of str is 142");
                exit(-1);
            }
        }
        sh1106_put_char_in_disply(str[i], display, page, &cursor);
    }
} 

#endif

uint8_t is_reversed = 0;

void sh1106_init(sh1106_t *display) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (display->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // command stream
    i2c_master_write_byte(cmd, 0xAE, true); // off
    i2c_master_write_byte(cmd, 0xD5, true); // clock div
    i2c_master_write_byte(cmd, 0x80, true);
    i2c_master_write_byte(cmd, 0xA8, true); // multiplex
    i2c_master_write_byte(cmd, 0x3f, true);
    i2c_master_write_byte(cmd, 0x8D, true); // charge pump
    i2c_master_write_byte(cmd, 0x14, true);
    i2c_master_write_byte(cmd, 0x10, true); // high column
    i2c_master_write_byte(cmd, 0xB0, true);
    i2c_master_write_byte(cmd, 0xC8, true);
    i2c_master_write_byte(cmd, 0x00, true); // low column
    i2c_master_write_byte(cmd, 0x10, true);
    i2c_master_write_byte(cmd, 0x40, true);
    i2c_master_write_byte(cmd, 0xA1, true); // segment remap
    i2c_master_write_byte(cmd, 0xA6, true);
    i2c_master_write_byte(cmd, 0x81, true); // contrast
    i2c_master_write_byte(cmd, 0xFF, true);
    i2c_master_write_byte(cmd, 0xAF, true); // on
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(display->port, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void sh1106_write_page(sh1106_t *display, uint8_t page) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (display->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x80, true); // single command
    i2c_master_write_byte(cmd, 0xB0 + page, true);
    i2c_master_write_byte(cmd, 0x40, true); // data stream
    i2c_master_write(cmd, display->pages[page], 128, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(display->port, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void sh1106_update(sh1106_t *display) {
    for (uint8_t i = 0; i < 8; i++)
        sh1106_write_page(display, i);
}

void sh1106_clear(sh1106_t *display) {
    memset(display->pages, 0x00, 1024); // 8 * 128, pixel count
}

void sh1106_fill(sh1106_t *display) {
    memset(display->pages, 0x00, 1024); // 8 * 128, pixel count
}

void sh1106_set(sh1106_t *display, uint8_t x, uint8_t y, bool s) {
    const uint8_t i = y / 8;
    if (s) {
        display->pages[i][x] |= (1 << (y % 8));
    } else {
        display->pages[i][x] &= ~(1 << (y % 8));
    }
}


bool sh1106_get(sh1106_t *display, uint8_t x, uint8_t y) {
    return display->pages[y / 8][x] & (1 << (y % 8));
}

bool i2c_address_scanner(uint8_t i2c_port, i2c_config_t i2c_conf) {
    bool address_found = false;
    esp_err_t rc = ESP_OK;

    ESP_ERROR_CHECK(i2c_param_config(i2c_port, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(i2c_port, I2C_MODE_MASTER,
                                       0, 0, 0));
    for (int i = 0; i < 127; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();

        ESP_ERROR_CHECK(i2c_master_start(cmd));
        ESP_ERROR_CHECK(i2c_master_write_byte(cmd, i, true));
        ESP_ERROR_CHECK(i2c_master_stop(cmd));
        if ((rc = i2c_master_cmd_begin(i2c_port, cmd,
                                 10 / portTICK_PERIOD_MS)) == ESP_OK)
        {
            address_found = true;
            printf("address = %#04x\n", i);
        }
        else if (rc != ESP_ERR_TIMEOUT && rc != ESP_FAIL)
            printf("error number %#04x\n", rc);
        i2c_cmd_link_delete(cmd);
    }
    i2c_driver_delete(i2c_port);
    if (address_found == true)
        return true;
    else {
        printf("No address found\n");
        return false;
    }
}

void sh1106_send_cmd(sh1106_t *display, uint8_t cmd_byte) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (display->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // command stream
    i2c_master_write_byte(cmd, cmd_byte, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(display->port, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

/* You should clean display and write on it again
* Did not find better solution because of bug, could not create another 
* sh1106_t variable
*/
void sh1106_reverse(sh1106_t *display) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create(); // command link initialize.

    i2c_master_start(cmd); // start bit.
    i2c_master_write_byte(cmd, (display->addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // command stream
    if (is_reversed == 0) {
        i2c_master_write_byte(cmd, 0xC0, true);
        i2c_master_write_byte(cmd, 0xA0, true);
        is_reversed = 1;
    }
    else {
        i2c_master_write_byte(cmd, 0xC8, true);
        i2c_master_write_byte(cmd, 0xA1, true);
        is_reversed = 0;
    }
    i2c_master_stop(cmd); // stop bit.
    ESP_ERROR_CHECK(i2c_master_cmd_begin(display->port, cmd, 10 / portTICK_PERIOD_MS));
    i2c_cmd_link_delete(cmd);

    sh1106_update(display);
}

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

void OLED_power_on(void) {
    gpio_set_direction(EN_OLED, GPIO_MODE_OUTPUT);
    gpio_set_level(EN_OLED, 1);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

