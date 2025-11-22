#include "oled.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "pins.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "OLED";
static uint8_t oled_buffer[OLED_WIDTH * OLED_HEIGHT / 8];
static uint8_t oled_addr = 0;
static bool oled_initialized = false;

static esp_err_t oled_write_cmd(uint8_t cmd) {
  i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
  i2c_master_start(cmd_handle);
  i2c_master_write_byte(cmd_handle, (oled_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd_handle, 0x00, true); // Control byte: command
  i2c_master_write_byte(cmd_handle, cmd, true);
  i2c_master_stop(cmd_handle);
  esp_err_t ret =
      i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(cmd_handle);
  return ret;
}

static esp_err_t oled_write_data(uint8_t *data, size_t len) {
  i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
  i2c_master_start(cmd_handle);
  i2c_master_write_byte(cmd_handle, (oled_addr << 1) | I2C_MASTER_WRITE, true);
  i2c_master_write_byte(cmd_handle, 0x40, true); // Control byte: data
  i2c_master_write(cmd_handle, data, len, true);
  i2c_master_stop(cmd_handle);
  esp_err_t ret =
      i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, pdMS_TO_TICKS(100));
  i2c_cmd_link_delete(cmd_handle);
  return ret;
}

static esp_err_t oled_detect_address(void) {
  // Try common OLED addresses
  uint8_t addresses[] = {OLED_ADDR_1, OLED_ADDR_2};

  for (int i = 0; i < 2; i++) {
    oled_addr = addresses[i];
    esp_err_t ret = oled_write_cmd(OLED_CMD_DISPLAY_OFF);
    if (ret == ESP_OK) {
      ESP_LOGI(TAG, "OLED found at address 0x%02X", oled_addr);
      return ESP_OK;
    }
  }

  ESP_LOGE(TAG, "OLED not found at any address");
  return ESP_FAIL;
}

esp_err_t oled_init(void) {
  if (oled_initialized)
    return ESP_OK;

  // Detect OLED address
  esp_err_t ret = oled_detect_address();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to detect OLED");
    return ret;
  }

  // Initialize OLED
  uint8_t init_commands[] = {OLED_CMD_DISPLAY_OFF,
                             OLED_CMD_SET_DISPLAY_CLK_DIV,
                             0x80,
                             OLED_CMD_SET_MULTIPLEX,
                             0x3F,
                             OLED_CMD_SET_DISPLAY_OFFSET,
                             0x00,
                             OLED_CMD_SET_START_LINE | 0x0,
                             OLED_CMD_CHARGE_PUMP,
                             0x14,
                             OLED_CMD_MEMORY_MODE,
                             0x00,
                             OLED_CMD_SEG_REMAP | 0x1,
                             OLED_CMD_COM_SCAN_DEC,
                             OLED_CMD_SET_COM_PINS,
                             0x12,
                             OLED_CMD_SET_CONTRAST,
                             0xCF,
                             OLED_CMD_SET_PRECHARGE,
                             0xF1,
                             OLED_CMD_SET_VCOM_DETECT,
                             0x40,
                             OLED_CMD_DISPLAY_ALL_ON_RESUME,
                             OLED_CMD_NORMAL_DISPLAY,
                             OLED_CMD_DISPLAY_ON};

  for (int i = 0; i < sizeof(init_commands); i++) {
    ret = oled_write_cmd(init_commands[i]);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to send init command %d", i);
      return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  // Clear display
  memset(oled_buffer, 0, sizeof(oled_buffer));
  oled_display();

  oled_initialized = true;
  ESP_LOGI(TAG, "OLED initialized successfully");
  return ESP_OK;
}

esp_err_t oled_clear(void) {
  if (!oled_initialized)
    return ESP_FAIL;
  memset(oled_buffer, 0, sizeof(oled_buffer));
  return ESP_OK;
}

esp_err_t oled_display(void) {
  if (!oled_initialized)
    return ESP_FAIL;

  uint8_t commands[] = {OLED_CMD_COLUMN_ADDR, 0, OLED_WIDTH - 1,
                        OLED_CMD_PAGE_ADDR,   0, (OLED_HEIGHT / 8) - 1};

  for (int i = 0; i < sizeof(commands); i++) {
    esp_err_t ret = oled_write_cmd(commands[i]);
    if (ret != ESP_OK)
      return ret;
  }

  return oled_write_data(oled_buffer, sizeof(oled_buffer));
}

esp_err_t oled_set_cursor(uint8_t x, uint8_t y) {
  if (!oled_initialized || x >= OLED_WIDTH || y >= OLED_HEIGHT)
    return ESP_FAIL;
  // Cursor position is handled in print functions
  return ESP_OK;
}

esp_err_t oled_draw_pixel(uint8_t x, uint8_t y, bool color) {
  if (!oled_initialized || x >= OLED_WIDTH || y >= OLED_HEIGHT)
    return ESP_FAIL;

  uint16_t index = x + (y / 8) * OLED_WIDTH;
  uint8_t bit = y % 8;

  if (color) {
    oled_buffer[index] |= (1 << bit);
  } else {
    oled_buffer[index] &= ~(1 << bit);
  }
  return ESP_OK;
}

esp_err_t oled_print(const char *str) {
  if (!oled_initialized || !str)
    return ESP_FAIL;

  // Simple 8x8 font rendering (basic implementation)
  uint8_t x = 0, y = 0;
  for (int i = 0; str[i] != '\0' && x < OLED_WIDTH - 8; i++) {
    char c = str[i];
    if (c == '\n') {
      y += 8;
      x = 0;
      continue;
    }

    // Draw simple 8x8 character (basic font)
    for (int px = 0; px < 8; px++) {
      for (int py = 0; py < 8; py++) {
        // Simple character pattern (you can improve this)
        bool pixel = (c >= 32 && c <= 126); // Basic printable chars
        oled_draw_pixel(x + px, y + py, pixel);
      }
    }
    x += 8;
  }
  return ESP_OK;
}

esp_err_t oled_println(const char *str) {
  esp_err_t ret = oled_print(str);
  if (ret == ESP_OK) {
    ret = oled_print("\n");
  }
  return ret;
}

esp_err_t oled_draw_line(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1,
                         bool color) {
  if (!oled_initialized)
    return ESP_FAIL;

  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;
  int err = dx - dy;

  while (true) {
    oled_draw_pixel(x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    int e2 = 2 * err;
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
  return ESP_OK;
}

esp_err_t oled_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                         bool color) {
  if (!oled_initialized)
    return ESP_FAIL;

  oled_draw_line(x, y, x + w - 1, y, color);
  oled_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
  oled_draw_line(x + w - 1, y + h - 1, x, y + h - 1, color);
  oled_draw_line(x, y + h - 1, x, y, color);
  return ESP_OK;
}

esp_err_t oled_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h,
                         bool color) {
  if (!oled_initialized)
    return ESP_FAIL;

  for (uint8_t i = x; i < x + w; i++) {
    for (uint8_t j = y; j < y + h; j++) {
      oled_draw_pixel(i, j, color);
    }
  }
  return ESP_OK;
}

