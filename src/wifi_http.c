#include "wifi_http.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "gps_parser.h"
#include "nvs_flash.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "WIFIHTTP";

static esp_err_t root_get_handler(httpd_req_t *req) {
  const char *html =
      "<!DOCTYPE html>"
      "<html><head>"
      "<meta charset='UTF-8'>"
      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
      "<title>OLEDGPS Tracker</title>"
      "<link rel='stylesheet' "
      "href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css'/>"
      "<style>"
      "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }"
      "#map { height: 400px; width: 100%; border: 1px solid #ccc; }"
      ".info { background: #f0f0f0; padding: 10px; margin: 10px 0; "
      "border-radius: 5px; }"
      ".status { font-weight: bold; }"
      ".online { color: green; }"
      ".offline { color: red; }"
      "</style>"
      "</head><body>"
      "<h1>🛰️ OLEDGPS Tracker</h1>"
      "<div class='info'>"
      "<div>Status: <span id='status' class='status'>Carregando...</span></div>"
      "<div>Satélites: <span id='satellites'>-</span></div>"
      "<div>Velocidade: <span id='speed'>-</span> km/h</div>"
      "<div>Altitude: <span id='altitude'>-</span> m</div>"
      "<div>Última atualização: <span id='lastUpdate'>-</span></div>"
      "</div>"
      "<div id='map'></div>"
      "<script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'></script>"
      "<script>"
      "let map, marker, polyline, positions = [];"
      "let lastLat = null, lastLon = null;"
      ""
      "function initMap() {"
      "  map = L.map('map').setView([-23.5505, -46.6333], 13);"
      "  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {"
      "    attribution: '© OpenStreetMap contributors'"
      "  }).addTo(map);"
      "  marker = L.marker([0, 0]).addTo(map);"
      "  polyline = L.polyline([], {color: 'red'}).addTo(map);"
      "}"
      ""
      "function updateGPS() {"
      "  fetch('/api/gps')"
      "    .then(response => response.json())"
      "    .then(data => {"
      "      document.getElementById('satellites').textContent = "
      "data.satellites || 0;"
      "      document.getElementById('speed').textContent = (data.speed || "
      "0).toFixed(1);"
      "      document.getElementById('altitude').textContent = (data.altitude "
      "|| 0).toFixed(1);"
      "      document.getElementById('lastUpdate').textContent = new "
      "Date().toLocaleTimeString();"
      "      "
      "      if (data.valid && data.latitude && data.longitude) {"
      "        document.getElementById('status').textContent = 'Conectado';"
      "        document.getElementById('status').className = 'status online';"
      "        "
      "        let lat = parseFloat(data.latitude);"
      "        let lon = parseFloat(data.longitude);"
      "        "
      "        if (lastLat !== lat || lastLon !== lon) {"
      "          marker.setLatLng([lat, lon]);"
      "          map.setView([lat, lon], 15);"
      "          "
      "          positions.push([lat, lon]);"
      "          polyline.setLatLngs(positions);"
      "          "
      "          lastLat = lat;"
      "          lastLon = lon;"
      "        }"
      "      } else {"
      "        document.getElementById('status').textContent = 'Sem sinal GPS';"
      "        document.getElementById('status').className = 'status offline';"
      "      }"
      "    })"
      "    .catch(error => {"
      "      document.getElementById('status').textContent = 'Erro de conexão';"
      "      document.getElementById('status').className = 'status offline';"
      "    });"
      "}"
      ""
      "initMap();"
      "setInterval(updateGPS, 2000);"
      "updateGPS();"
      "</script>"
      "</body></html>";

  httpd_resp_set_type(req, "text/html");
  httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

static esp_err_t gps_api_handler(httpd_req_t *req) {
  gps_data_t *gps = gps_get_data();

  char json_response[512];
  snprintf(json_response, sizeof(json_response),
           "{"
           "\"valid\":%s,"
           "\"latitude\":%.8f,"
           "\"longitude\":%.8f,"
           "\"altitude\":%.2f,"
           "\"satellites\":%d,"
           "\"speed\":%.2f,"
           "\"course\":%.2f,"
           "\"timestamp\":\"%s\","
           "\"date\":\"%s\""
           "}",
           gps->valid ? "true" : "false", gps->latitude, gps->longitude,
           gps->altitude, gps->satellites, gps->speed, gps->course,
           gps->timestamp, gps->date);

  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_send(req, json_response, HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

esp_err_t http_server_start(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;
  esp_err_t ret = httpd_start(&server, &config);
  if (ret != ESP_OK)
    return ret;
  httpd_uri_t root = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = root_get_handler,
      .user_ctx = NULL,
  };
  httpd_register_uri_handler(server, &root);

  httpd_uri_t gps_api = {
      .uri = "/api/gps",
      .method = HTTP_GET,
      .handler = gps_api_handler,
      .user_ctx = NULL,
  };
  httpd_register_uri_handler(server, &gps_api);

  return ESP_OK;
}

esp_err_t wifi_init_apsta(const char *ap_ssid, const char *ap_pass) {
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_ap_config = {0};
  strncpy((char *)wifi_ap_config.ap.ssid, ap_ssid,
          sizeof(wifi_ap_config.ap.ssid) - 1);
  wifi_ap_config.ap.ssid_len = strlen(ap_ssid);
  wifi_ap_config.ap.channel = 1;
  wifi_ap_config.ap.max_connection = 4;
  wifi_ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
  strncpy((char *)wifi_ap_config.ap.password, ap_pass,
          sizeof(wifi_ap_config.ap.password) - 1);
  if (strlen(ap_pass) == 0) {
    wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  wifi_config_t wifi_sta_config = {0};
  // STA credentials can be set later via provisioning; keep empty for now

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(TAG, "WiFi AP started: %s", ap_ssid);
  return ESP_OK;
}
