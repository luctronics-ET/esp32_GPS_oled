#include "driver/i2c.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gps_parser.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "oled.h"
#include "pins.h"
#include "sdmmc_cmd.h"
#include "wifi_http.h"
#include <stdio.h>

static const char *TAG = "OLEDGPS";

static esp_err_t init_nvs(void) {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  return err;
}

static esp_err_t init_i2c(void) {
  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = OLED_SDA_GPIO,
      .scl_io_num = OLED_SCL_GPIO,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = 400000,
      .clk_flags = 0,
  };
  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
  return i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
}

static void i2c_scan(void) {
  ESP_LOGI(TAG, "Scanning I2C bus on SDA=%d SCL=%d", OLED_SDA_GPIO,
           OLED_SCL_GPIO);
  int found = 0;
  for (uint8_t addr = 0x03; addr < 0x78; addr++) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(20));
    i2c_cmd_link_delete(cmd);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "I2C device found at 0x%02X", addr);
      found++;
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  if (!found) {
    ESP_LOGW(TAG, "No I2C devices found. Check wiring, power, and pull-ups.");
  } else {
    ESP_LOGI(TAG, "I2C scan complete: %d device(s) found.", found);
  }
}

static esp_err_t init_uart_gps(void) {
  const uart_config_t uart_config = {
      .baud_rate = 9600,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 2048, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
  // Map pins (note: may conflict with USB-Serial on 20/21)
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, GPS_TX_GPIO, GPS_RX_GPIO,
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  return ESP_OK;
}

static esp_err_t mount_sdcard(void) {
  // Use SD over SPI (SPI2_HOST on ESP32-C3)
  spi_bus_config_t bus_cfg = {
      .mosi_io_num = SD_MOSI_GPIO,
      .miso_io_num = SD_MISO_GPIO,
      .sclk_io_num = SD_SCK_GPIO,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
  };
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();
  host.slot = SPI2_HOST;

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = SD_CS_GPIO;
  slot_config.host_id = host.slot;

  esp_vfs_fat_sdspi_mount_config_t mount_config = {
      .base_path = "/sd",
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  sdmmc_card_t *card;
  esp_err_t ret =
      esp_vfs_fat_sdspi_mount("/sd", &host, &slot_config, &mount_config, &card);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG,
             "SD card not mounted (no card or wiring). Continuing without SD. "
             "err=%s",
             esp_err_to_name(ret));
    return ESP_OK; // allow running without SD card
  }
  sdmmc_card_print_info(stdout, card);
  return ESP_OK;
}

static void display_gps_info(void) {
  gps_data_t *gps = gps_get_data();

  oled_clear();

  if (gps_has_fix()) {
    char line1[32], line2[32], line3[32], line4[32];

    // Line 1: Satellites
    snprintf(line1, sizeof(line1), "SAT: %d", gps->satellites);
    oled_print(line1);
    oled_println("");

    // Line 2: Coordinates
    snprintf(line2, sizeof(line2), "LAT: %.6f", gps->latitude);
    oled_print(line2);
    oled_println("");

    snprintf(line3, sizeof(line3), "LON: %.6f", gps->longitude);
    oled_print(line3);
    oled_println("");

    // Line 3: Speed and Altitude
    snprintf(line4, sizeof(line4), "SPD: %.1f ALT: %.1f", gps->speed,
             gps->altitude);
    oled_print(line4);
  } else {
    oled_print("GPS: Searching...");
    oled_println("");
    oled_print("Sats: ");
    char sats[8];
    snprintf(sats, sizeof(sats), "%d", gps->satellites);
    oled_print(sats);
  }

  oled_display();
}

static esp_err_t save_gps_to_sd(void) {
  gps_data_t *gps = gps_get_data();
  if (!gps->valid)
    return ESP_OK; // Only save valid GPS data

  FILE *file = fopen("/sd/gps_log.txt", "a");
  if (!file) {
    ESP_LOGW(TAG, "Failed to open SD file for writing");
    return ESP_FAIL;
  }

  fprintf(file, "%lu,%.8f,%.8f,%.2f,%d,%.2f,%.2f,%s,%s\n",
          (unsigned long)(esp_timer_get_time() / 1000000), gps->latitude,
          gps->longitude, gps->altitude, gps->satellites, gps->speed,
          gps->course, gps->timestamp, gps->date);

  fclose(file);
  ESP_LOGI(TAG, "GPS data saved to SD");
  return ESP_OK;
}

void app_main(void) {
  ESP_ERROR_CHECK(init_nvs());
  ESP_ERROR_CHECK(init_i2c());
  i2c_scan();
  ESP_ERROR_CHECK(init_uart_gps());
  ESP_ERROR_CHECK(mount_sdcard());
  ESP_ERROR_CHECK(wifi_init_apsta("OLEDGPS", "12345678"));
  ESP_ERROR_CHECK(http_server_start());
  ESP_ERROR_CHECK(mqtt_init());

  // Initialize OLED
  esp_err_t oled_ret = oled_init();
  if (oled_ret == ESP_OK) {
    ESP_LOGI(TAG, "OLED initialized");
    oled_clear();
    oled_print("OLEDGPS Starting...");
    oled_display();
  } else {
    ESP_LOGW(TAG, "OLED not available");
  }

  ESP_LOGI(TAG, "Init complete. Reading GPS...");
  uint8_t buf[128];
  uint32_t last_display_update = 0;
  uint32_t last_mqtt_publish = 0;
  uint32_t last_sd_save = 0;

  while (1) {
    int len =
        uart_read_bytes(UART_NUM_0, buf, sizeof(buf) - 1, pdMS_TO_TICKS(1000));
    if (len > 0) {
      buf[len] = 0;
      ESP_LOGI(TAG, "GPS: %s", (char *)buf);

      // Parse NMEA sentence
      gps_parse_nmea((char *)buf);

      uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

      // Update display every 2 seconds
      if (now - last_display_update > 2000) {
        if (oled_ret == ESP_OK) {
          display_gps_info();
        }
        last_display_update = now;
      }

      // Try MQTT connection and publish every 10 seconds
      if (now - last_mqtt_publish > 10000) {
        mqtt_connect();
        mqtt_publish_gps_data();
        last_mqtt_publish = now;
      }

      // Save to SD every 5 seconds (if valid GPS data)
      if (now - last_sd_save > 5000) {
        save_gps_to_sd();
        last_sd_save = now;
      }
    }
  }
}
