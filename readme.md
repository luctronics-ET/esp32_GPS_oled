# esp32_GPS_oled

Projeto de rastreador GPS com OLED para **ESP32-C3 (ESP-IDF)** e suporte legado a **ESP8266 NodeMCU (Arduino)**. Exibe dados no OLED, serve uma UI web com mapa (Leaflet), publica JSON via MQTT e grava log em SD (SPI), quando disponível.

## Visão Geral
- Fluxo principal (ESP32-C3):
  - Inicializa NVS, I2C (OLED), UART (GPS), SPI (SD), WiFi AP+STA, HTTP server e MQTT.
  - Lê sentenças NMEA do GPS em `UART0`, processa em `src/gps_parser.c`.
  - Atualiza OLED a cada ~2s, publica MQTT a cada ~10s, grava SD a cada ~5s.
  - UI HTTP: endpoint `/api/gps` (JSON) e página com mapa (Leaflet) atualizando a cada 2s.
- Tolerante a periféricos ausentes: se OLED/SD não estiverem presentes, o sistema segue executando.

## Pinagem (Resumo)
Os pinos são configuráveis via `build_flags` em [platformio.ini](platformio.ini). Defaults em [include/pins.h](include/pins.h).

### ESP32-C3 (ESP-IDF)
- OLED (I2C): `OLED_SDA_GPIO=8`, `OLED_SCL_GPIO=9`
- SD (SPI): `SD_CS_GPIO=4`, `SD_SCK_GPIO=6`, `SD_MOSI_GPIO=7`, `SD_MISO_GPIO=5`
- GPS (UART0): `GPS_RX_GPIO=20`, `GPS_TX_GPIO=21`

Observação UART0: pode haver conflito com USB-Serial nos GPIO 20/21. Se o monitor/serial ficar instável:
- Use `UART1` com pinos disponíveis e ajuste `uart_set_pin()` em `src/main.c`, ou
- Mova o GPS para pinos que não conflitem com a CDC/USB da placa.

### ESP8266 NodeMCU (Arduino)
- OLED (I2C): `OLED_SDA_GPIO=D2`, `OLED_SCL_GPIO=D1`
- GPS (UART): `GPS_RX_GPIO=D4`, `GPS_TX_GPIO=D3`

## Contrato de Dados
- Estrutura `gps_data_t` (em `include/gps_parser.h`): `valid, latitude, longitude, altitude, satellites, speed(km/h), course, timestamp(HHMMSS), date(DDMMYY)`.
- HTTP `/api/gps` (em `src/wifi_http.c`): JSON com campos estáveis — `valid, latitude, longitude, altitude, satellites, speed, course, timestamp, date`.
- MQTT `gps/tracker` (em `src/mqtt_client.c`): JSON com `device_id, timestamp(unix), valid, latitude, longitude, altitude, satellites, speed, course, gps_time, gps_date`. QoS 1.
- Gating de rede: ações MQTT só ocorrem quando `is_server_network()` detecta rede `192.168.1.x`.

## Build & Upload
Requer **PlatformIO**.

### ESP32-C3 (ESP-IDF)
```sh
pio run -e esp32c3
pio run -e esp32c3 -t upload
pio device monitor -b 115200
```
Ajuste pinos/macros em `platformio.ini` se necessário. SDK config: `sdkconfig.esp32c3` (não editar casualmente).

### ESP8266 NodeMCU (Arduino legado)
```sh
pio run -e nodemcu
pio run -e nodemcu -t upload
pio device monitor -b 115200
```
Dependências Arduino via `lib_deps` em `platformio.ini` (Adafruit SSD1306/GFX, TinyGPSPlus).

## Execução (ESP32-C3)
- Ao iniciar, o AP WiFi `OLEDGPS` é criado (senha `12345678`).
- Acesse a UI web na raiz (`/`) hospedada pelo dispositivo; ela utiliza Leaflet e consulta `/api/gps` a cada 2s.
- Se um SD estiver presente, logs são gravados em `/sd/gps_log.txt`.

## Configuração MQTT
- Ajuste `MQTT_BROKER_HOST` e `MQTT_BROKER_PORT` em `include/mqtt_client.h`.
- Para redes diferentes de `192.168.1.x`, atualize a lógica de `is_server_network()` em `src/mqtt_client.c`.

## Estrutura do Código
- `src/main.c`: orquestra inicializações e laço principal, cadências e chamadas periódicas.
- `src/gps_parser.c`: parse básico de `$GPGGA` e `$GPRMC` com `parse_coordinate()`.
- `src/oled.c`: driver simples SSD1306-like (I2C), autodetecção `0x3C/0x3D`.
- `src/wifi_http.c`: servidor HTTP (página e API JSON), CORS `*`.
- `src/mqtt_client.c`: cliente MQTT com publish condicionado por rede.
- `include/*.h`: pinos, tipos e configurações.

## Hardware (Ligaçãos e Esquemas)

### OLED SSD1306 (I2C)
ESP32-C3 (ESP-IDF):

```
ESP32-C3           OLED SSD1306
3V3      --------> VCC
GND      --------> GND
GPIO8    --------> SDA
GPIO9    --------> SCL
```

NodeMCU (Arduino):

```
NodeMCU            OLED SSD1306
3V3      --------> VCC
GND      --------> GND
D2       --------> SDA
D1       --------> SCL
```

### SD Card (SPI)
ESP32-C3:

```
ESP32-C3           SD (SPI)
GPIO4   (CS)   --> CS
GPIO6   (SCK)  --> SCK
GPIO7   (MOSI) --> MOSI
GPIO5   (MISO) <-- MISO
3V3/GND         -> VCC/GND
```

### GPS (UART)
ESP32-C3 (UART0):

```
ESP32-C3           GPS Módulo (ex: NEO-6M)
GPIO21 (TX0)   --> RX (do GPS)
GPIO20 (RX0)   <-- TX (do GPS)
3V3/GND         -> VCC/GND
```

NodeMCU:

```
NodeMCU            GPS Módulo
D3 (TX)        --> RX (do GPS)
D4 (RX)        <-- TX (do GPS)
3V3/GND        -> VCC/GND
```

Notas:
- Utilize módulos compatíveis com 3.3V. SD e OLED tipicamente operam em 3.3V.
- Em ESP32-C3, `UART0` nos GPIO 20/21 pode conflitar com USB-Serial. Se houver instabilidade, considere `UART1` e ajuste `uart_set_pin()` em `src/main.c` ou remapeie pinos.

### Diagrama simples de arquitetura

```
GPS (UART) -> Parser (gps_parser) ->
  -> OLED (I2C)
  -> HTTP /api/gps (JSON + UI Leaflet)
  -> MQTT (rede 192.168.1.x)
  -> SD Log (/sd/gps_log.txt)
```

## Dicas de Troubleshooting
- OLED não exibe: confira SDA/SCL, pull-ups e endereço (`0x3C/0x3D`). Use o scan I2C de `main.c`.
- SD não monta: o sistema continua; verifique fiação (CS/SCK/MOSI/MISO) e alimentação.
- GPS sem fix: `gps_has_fix()` exige `valid && satellites>=3`; aguarde céu aberto.
- MQTT não publica: confirme conexão STA e IP na faixa `192.168.1.x`; ajuste broker/IP.

## Licença
Consulte [LICENSE](LICENSE).
