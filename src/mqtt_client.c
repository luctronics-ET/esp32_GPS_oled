#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_mqtt.h"
#include "esp_wifi.h"
#include "gps_parser.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  esp_mqtt_event_handle_t event = event_data;

  switch (event->event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT connected");
    mqtt_connected = true;
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT disconnected");
    mqtt_connected = false;
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGD(TAG, "MQTT message published");
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGE(TAG, "MQTT error");
    mqtt_connected = false;
    break;
  default:
    break;
  }
}

static bool is_server_network(void) {
  esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
  if (!netif)
    return false;

  esp_netif_ip_info_t ip_info;
  esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
  if (ret != ESP_OK)
    return false;

  // Check if connected to server network (adjust IP range as needed)
  uint32_t ip = ip_info.ip.addr;
  uint8_t ip_bytes[4];
  ip_bytes[0] = ip & 0xFF;
  ip_bytes[1] = (ip >> 8) & 0xFF;
  ip_bytes[2] = (ip >> 16) & 0xFF;
  ip_bytes[3] = (ip >> 24) & 0xFF;

  // Check if on server network (e.g., 192.168.1.x)
  return (ip_bytes[0] == 192 && ip_bytes[1] == 168 && ip_bytes[2] == 1);
}

esp_err_t mqtt_init(void) {
  esp_mqtt_client_config_t mqtt_cfg = {
      .broker.address.hostname = MQTT_BROKER_HOST,
      .broker.address.port = MQTT_BROKER_PORT,
  };

  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  if (!mqtt_client) {
    ESP_LOGE(TAG, "Failed to initialize MQTT client");
    return ESP_FAIL;
  }

  esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID,
                                 mqtt_event_handler, NULL);

  ESP_LOGI(TAG, "MQTT client initialized");
  return ESP_OK;
}

esp_err_t mqtt_connect(void) {
  if (!mqtt_client) {
    ESP_LOGE(TAG, "MQTT client not initialized");
    return ESP_FAIL;
  }

  if (!is_server_network()) {
    ESP_LOGD(TAG, "Not on server network, skipping MQTT connection");
    return ESP_OK;
  }

  esp_err_t ret = esp_mqtt_client_start(mqtt_client);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(ret));
    return ret;
  }

  ESP_LOGI(TAG, "MQTT client started");
  return ESP_OK;
}

esp_err_t mqtt_disconnect(void) {
  if (mqtt_client) {
    esp_mqtt_client_stop(mqtt_client);
    mqtt_connected = false;
  }
  return ESP_OK;
}

esp_err_t mqtt_publish_gps_data(void) {
  if (!mqtt_connected || !is_server_network()) {
    return ESP_OK; // Not connected or not on server network
  }

  gps_data_t *gps = gps_get_data();

  char json_payload[512];
  snprintf(json_payload, sizeof(json_payload),
           "{"
           "\"device_id\":\"oledgps\","
           "\"timestamp\":%lu,"
           "\"valid\":%s,"
           "\"latitude\":%.8f,"
           "\"longitude\":%.8f,"
           "\"altitude\":%.2f,"
           "\"satellites\":%d,"
           "\"speed\":%.2f,"
           "\"course\":%.2f,"
           "\"gps_time\":\"%s\","
           "\"gps_date\":\"%s\""
           "}",
           (unsigned long)(esp_timer_get_time() / 1000000), // Unix timestamp
           gps->valid ? "true" : "false", gps->latitude, gps->longitude,
           gps->altitude, gps->satellites, gps->speed, gps->course,
           gps->timestamp, gps->date);

  int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_GPS,
                                       json_payload, 0, 1, 0);
  if (msg_id < 0) {
    ESP_LOGE(TAG, "Failed to publish GPS data");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "GPS data published to MQTT");
  return ESP_OK;
}

esp_err_t mqtt_publish_status(const char *status) {
  if (!mqtt_connected || !is_server_network()) {
    return ESP_OK;
  }

  int msg_id =
      esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_STATUS, status, 0, 1, 0);
  if (msg_id < 0) {
    ESP_LOGE(TAG, "Failed to publish status");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Status published: %s", status);
  return ESP_OK;
}

bool mqtt_is_connected(void) { return mqtt_connected && is_server_network(); }

