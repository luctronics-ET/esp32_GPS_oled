#pragma once

#include "esp_err.h"

// MQTT configuration
#define MQTT_BROKER_HOST "192.168.1.100" // Change to your MQTT broker IP
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID "oledgps_%d"
#define MQTT_TOPIC_GPS "gps/tracker"
#define MQTT_TOPIC_STATUS "gps/status"

// Function prototypes
esp_err_t mqtt_init(void);
esp_err_t mqtt_connect(void);
esp_err_t mqtt_disconnect(void);
esp_err_t mqtt_publish_gps_data(void);
esp_err_t mqtt_publish_status(const char *status);
bool mqtt_is_connected(void);

