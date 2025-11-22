#pragma once

#include "esp_err.h"

// OLED display dimensions
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// I2C address (try both common addresses)
#define OLED_ADDR_1 0x3C
#define OLED_ADDR_2 0x3D

// Display commands
#define OLED_CMD_SET_CONTRAST 0x81
#define OLED_CMD_DISPLAY_ALL_ON_RESUME 0xA4
#define OLED_CMD_DISPLAY_ALL_ON 0xA5
#define OLED_CMD_NORMAL_DISPLAY 0xA6
#define OLED_CMD_INVERT_DISPLAY 0xA7
#define OLED_CMD_DISPLAY_OFF 0xAE
#define OLED_CMD_DISPLAY_ON 0xAF
#define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
#define OLED_CMD_SET_COM_PINS 0xDA
#define OLED_CMD_SET_VCOM_DETECT 0xDB
#define OLED_CMD_SET_DISPLAY_CLK_DIV 0xD5
#define OLED_CMD_SET_PRECHARGE 0xD9
#define OLED_CMD_SET_MULTIPLEX 0xA8
#define OLED_CMD_SET_LOW_COLUMN 0x00
#define OLED_CMD_SET_HIGH_COLUMN 0x10
#define OLED_CMD_SET_START_LINE 0x40
#define OLED_CMD_MEMORY_MODE 0x20
#define OLED_CMD_COLUMN_ADDR 0x21
#define OLED_CMD_PAGE_ADDR 0x22
#define OLED_CMD_COM_SCAN_INC 0xC0
#define OLED_CMD_COM_SCAN_DEC 0xC8
#define OLED_CMD_SEG_REMAP 0xA0
#define OLED_CMD_CHARGE_PUMP 0x8D

// Function prototypes
esp_err_t oled_init(void);
esp_err_t oled_clear(void);
esp_err_t oled_display(void);
esp_err_t oled_set_cursor(uint8_t x, uint8_t y);
esp_err_t oled_print(const char *str);
esp_err_t oled_println(const char *str);
esp_err_t oled_draw_pixel(uint8_t x, uint8_t y, bool color);
esp_err_t oled_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                         bool color);
esp_err_t oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                         bool color);
esp_err_t oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                         bool color);

