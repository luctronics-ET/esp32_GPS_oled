# Copilot Instructions for esp32_GPS_oled

Purpose: Help AI agents work productively in this ESP32-C3/ESP8266 GPS + OLED project. Keep changes minimal, consistent with existing style, and focused on this codebase’s patterns.

## Architecture Overview
- **Targets:** `ESP32-C3 (ESP-IDF)` primary, `ESP8266 NodeMCU (Arduino)` legacy. See `platformio.ini` for envs.
- **Main Loop:** [src/main.c](src/main.c) orchestrates init and an infinite loop:
  - NVS, I2C (`OLED`), UART (`GPS`), SPI (`SD`), WiFi AP+STA, HTTP server, MQTT.
  - Reads NMEA from `UART0`, feeds parser in [src/gps_parser.c](src/gps_parser.c), then periodically:
    - **OLED UI:** `display_gps_info()` via [src/oled.c](src/oled.c)
    - **HTTP API/UI:** `/api/gps` + root HTML in [src/wifi_http.c](src/wifi_http.c)
    - **MQTT:** conditioned on STA network check in [src/mqtt_client.c](src/mqtt_client.c)
    - **SD logging:** append to `/sd/gps_log.txt`
- **Pins & Config:** Centralized in [include/pins.h](include/pins.h) and overridden by `build_flags` in `platformio.ini`.

## Key Patterns & Conventions
- **ESP-IDF style:** Explicit `ESP_ERROR_CHECK(...)` init functions returning `esp_err_t`. Prefer small, single-purpose inits (`init_i2c`, `init_uart_gps`, etc.). All init calls must run in `app_main()` before main loop starts.
- **Periodic work cadence:** Use millisecond ticks (`xTaskGetTickCount()*portTICK_PERIOD_MS`) in the main loop to gate OLED (2s), MQTT (10s), SD (5s). Timing checks happen within the UART read loop in `app_main()` to avoid busy-waiting.
- **Network gating:** MQTT actions are no-ops unless `is_server_network()` detects `192.168.1.x` subnet. Mirror this behavior for any new network calls.
- **HTTP server:** Serve minimal inline HTML/JS with Leaflet map, CORS `*`, JSON from `/api/gps`. Keep payload fields aligned with `gps_data_t` structure—no extra fields.
- **OLED driver:** Simple I2C SSD1306-like protocol; auto-detect address (`0x3C` or `0x3D`). Write via `oled_write_cmd()/oled_write_data()`. Use `oled_buffer` (1024-byte bitmap), then `oled_display()` to update display.
- **GPS parsing:** Parse only `$GPGGA` (position/altitude/satellites/time) and `$GPRMC` (speed/course/date/status) sentences. Use `parse_coordinate()` helper; set `gps_data` fields directly. `gps_has_fix()` requires `valid && satellites>=3`.
- **Error tolerance:** SD card failure is silent (log warning, continue). OLED init failure logs warning but loop continues. WiFi/MQTT handle disconnects gracefully—main loop is not blocked.

## Developer Workflows
- **Build (ESP32-C3, ESP-IDF):**
  - Via PlatformIO:
    ```sh
    pio run -e esp32c3
    pio run -e esp32c3 -t upload
    pio device monitor -b 115200
    ```
  - Pins and macros come from `build_flags` in [platformio.ini](platformio.ini). Modify there, not in [include/pins.h](include/pins.h).
  - SDK config: `sdkconfig.esp32c3` (do not edit casually; PlatformIO manages it).
- **Build (ESP8266, Arduino legacy):**
  ```sh
  pio run -e nodemcu
  pio run -e nodemcu -t upload
  pio device monitor -b 115200
  ```
- **Logging:** Use `ESP_LOGI(TAG, "msg")`, `ESP_LOGW()`, `ESP_LOGE()` with module `TAG` strings: `OLEDGPS` (main), `GPS_PARSER`, `MQTT`, `WIFIHTTP`, `OLED`.
- **Monitoring:** `pio device monitor -b 115200` shows UART0 output and all `ESP_LOG*` messages. GPS NMEA sentences are logged as-is to help debug parsing.

## Data Flow & Update Cycle
1. **Receive:** GPS module → UART0 (9600 baud, one sentence per ~1 sec)
2. **Parse:** `uart_read_bytes()` fills buffer → `gps_parse_nmea()` updates global `gps_data`
3. **Distribute:** Single copy in memory; multiple readers (`gps_get_data()`) access it safely
4. **Update outputs:** 
   - Every 2s: OLED calls `display_gps_info()`, reads `gps_data`, renders to `oled_buffer`, calls `oled_display()`
   - Every 10s: MQTT calls `mqtt_publish_gps_data()`, builds JSON from `gps_get_data()`, publishes if connected
   - Every 5s: `save_gps_to_sd()` appends CSV line with timestamp, lat, lon, alt, sats, speed, course, time, date

## Integration Points
- **MQTT:** Config in [include/mqtt_client.h](include/mqtt_client.h): `MQTT_BROKER_HOST`, `MQTT_BROKER_PORT`, topics `gps/tracker`, `gps/status`. Publish JSON built from `gps_get_data()`; QoS 1. Only publishes if `mqtt_is_connected()` AND `is_server_network()` detects `192.168.1.x`.
- **HTTP UI:** Root handler in [src/wifi_http.c](src/wifi_http.c) serves Leaflet map; `/api/gps` endpoint returns JSON. Map polls every 2s. Field names must match `gps_data_t` exactly: `valid, latitude, longitude, altitude, satellites, speed, course, timestamp, date`. Frontend is embedded HTML/JS (no external files).
- **WiFi:** AP+STA initialized in `app_main()`. AP SSID is `OLEDGPS`, password `12345678` (hardcoded). STA attempts to connect based on saved credentials or defaults. Check `is_server_network()` return to gate MQTT/logging features.
- **GPS Module:** Outputs NMEA 0183 at 9600 baud. Device filters on `$GPGGA` and `$GPRMC` sentences only. Must output position (GGA) and speed (RMC) for valid fix.

## Data Structures & Fields
**`gps_data_t`** ([include/gps_parser.h](include/gps_parser.h)):
- `valid: bool` — True if GPS has valid signal (quality > 0 from GGA or status 'A' from RMC)
- `latitude: double` — Decimal degrees (N positive, S negative)
- `longitude: double` — Decimal degrees (E positive, W negative)
- `altitude: float` — Meters above sea level
- `satellites: uint8_t` — Number of satellites in use
- `speed: float` — Kilometers per hour (converted from knots via RMC)
- `course: float` — Degrees (0-359, from RMC)
- `timestamp: char[10]` — UTC time HHMMSS from GGA
- `date: char[7]` — UTC date DDMMYY from RMC

## Pin Mapping
**ESP32-C3 (esp-idf)** via [platformio.ini](platformio.ini):
- OLED (I2C): `OLED_SDA_GPIO=8`, `OLED_SCL_GPIO=9` (400 kHz)
- SD (SPI): `SD_CS_GPIO=4`, `SD_SCK_GPIO=6`, `SD_MOSI_GPIO=7`, `SD_MISO_GPIO=5`
- GPS (UART0): `GPS_RX_GPIO=20`, `GPS_TX_GPIO=21` (9600 baud)

**ESP8266 NodeMCU (Arduino)**:
- OLED (I2C): `OLED_SDA_GPIO=D2`, `OLED_SCL_GPIO=D1`
- GPS (UART): `GPS_RX_GPIO=D4`, `GPS_TX_GPIO=D3`

**Prefer modifying via `build_flags` in [platformio.ini](platformio.ini)**; [include/pins.h](include/pins.h) provides defaults.

## Stable JSON Contract
- **HTTP `/api/gps`** ([src/wifi_http.c](src/wifi_http.c)): `{valid, latitude, longitude, altitude, satellites, speed, course, timestamp, date}`. CORS: `*`. Frontend polls every 2s.
- **MQTT `gps/tracker`** ([src/mqtt_client.c](src/mqtt_client.c)): `{device_id, timestamp_unix, valid, latitude, longitude, altitude, satellites, speed, course, gps_time, gps_date}`. QoS 1. Publishes only if `mqtt_is_connected()` AND `is_server_network()` == true (192.168.1.x).

## Safe Changes & Examples
- **Add a new metric to API/MQTT:** Extend `gps_data_t` in [include/gps_parser.h](include/gps_parser.h), populate in [src/gps_parser.c](src/gps_parser.c), then update JSON builders in [src/wifi_http.c](src/wifi_http.c) and [src/mqtt_client.c](src/mqtt_client.c) consistently.
- **Adjust publish cadence:** Modify `last_display_update`, `last_mqtt_publish`, `last_sd_save` thresholds in [src/main.c](src/main.c) main loop; keep non-blocking execution.
- **Pins per board:** Prefer changing `build_flags` in [platformio.ini](platformio.ini) rather than editing [include/pins.h](include/pins.h).
- **Handle missing peripherals:** Always check init return codes and handle gracefully (see `mount_sdcard()`, `oled_init()`). Main loop must survive missing OLED/SD.

## Common Pitfalls
- **UART0 conflicts:** GPIO 20/21 may conflict with USB-Serial on some ESP32-C3 boards. If monitor/upload is flaky, try `UART1` or reroute GPS RX/TX.
- **Blocking calls in main loop:** Do NOT use `vTaskDelay()`, blocking WiFi calls, or synchronous I2C transactions without timeout. Timing must rely on periodic checks with `xTaskGetTickCount()`.
- **JSON field stability:** Changing `/api/gps` field names breaks the frontend map. Update both C code and JavaScript simultaneously.
- **GPS fix validation:** `gps_has_fix()` requires both `valid==true` AND `satellites>=3`. Incomplete fixes (only lat/lon) should not trigger MQTT/SD writes.
- **MQTT network gating:** Always check `is_server_network()` before MQTT publish. Publishing to broker outside `192.168.1.x` will fail silently without error logs.

## Guardrails for Agents
- Keep public function signatures stable unless updating all call sites (e.g., `gps_get_data()`, `display_gps_info()`, `mqtt_publish_gps_data()`).
- Avoid introducing new dependencies; use only ESP-IDF, POSIX file I/O, and standard C library calls already in use.
- Preserve error tolerance: SD card, OLED, and MQTT failures should log but not crash the main loop.
- Do not add blocking UI interactions or interrupt handlers that could stall the periodic work loop.

Feedback: If any workflow (build, upload, monitor) or integration detail is unclear, tell us what you’re missing, and we’ll refine this document.
