# 🛰️ OLEDGPS - ESP32-C3 GPS Tracker

Sistema completo de rastreamento GPS com display OLED, interface web e transmissão MQTT.

## 📋 Funcionalidades

- **GPS**: Leitura NMEA com parser para coordenadas, velocidade, altitude
- **Display OLED**: 128x64 I2C mostrando dados GPS em tempo real
- **Interface Web**: Mapa interativo com Leaflet.js e API REST
- **Armazenamento**: Log automático em cartão SD (CSV)
- **MQTT**: Transmissão de dados JSON quando conectado à rede do servidor
- **WiFi**: Modo AP+STA para acesso remoto

## 🔌 Pinout ESP32-C3 Super Mini

| Componente | Pinos | Função |
|------------|-------|--------|
| **OLED I2C** | SDA: GPIO8, SCL: GPIO9 | Display 128x64 |
| **SD Card SPI** | CS: GPIO4, MISO: GPIO5, SCK: GPIO6, MOSI: GPIO7 | Armazenamento |
| **GPS UART** | RX: GPIO20, TX: GPIO21 | Receptor GPS |

> ⚠️ **Atenção**: GPIO20/21 são usados pelo USB-Serial. Desconecte GPS durante programação.

## 🚀 Instalação e Uso

### 1. Compilar e Gravar

```bash
# Compilar
pio run

# Gravar firmware
pio run -t upload

# Monitor serial
pio device monitor -b 115200
```

### 2. Conectar Hardware

- **OLED**: SDA→GPIO8, SCL→GPIO9, VCC→3.3V, GND→GND
- **SD Card**: CS→GPIO4, MISO→GPIO5, SCK→GPIO6, MOSI→GPIO7, VCC→3.3V, GND→GND
- **GPS**: TX→GPIO20, RX→GPIO21, VCC→3.3V, GND→GND

### 3. Acesso Web

1. Conecte ao WiFi "OLEDGPS" (senha: 12345678)
2. Abra <http://192.168.4.1> no navegador
3. Visualize mapa, coordenadas e dados em tempo real

## 📊 Dados e APIs

### Endpoint REST

- **GET /api/gps**: Retorna dados GPS em JSON

```json
{
  "valid": true,
  "latitude": -23.5505,
  "longitude": -46.6333,
  "altitude": 760.5,
  "satellites": 8,
  "speed": 45.2,
  "course": 180.5,
  "timestamp": "143022",
  "date": "141025"
}
```

### Log SD Card

Arquivo: `/sd/gps_log.txt` (formato CSV)

```
timestamp,latitude,longitude,altitude,satellites,speed,course,gps_time,gps_date
1697203022,-23.55050000,-46.63330000,760.50,8,45.20,180.50,143022,141025
```

### MQTT

- **Broker**: 192.168.1.100:1883 (configurável)
- **Tópicos**:
  - `gps/tracker`: Dados GPS JSON
  - `gps/status`: Status do dispositivo
- **Ativação**: Automática quando conectado à rede do servidor

## ⚙️ Configuração

### MQTT Broker

Edite `include/mqtt_client.h`:

```c
#define MQTT_BROKER_HOST "192.168.1.100"  // IP do seu broker
#define MQTT_BROKER_PORT 1883
```

### Rede do Servidor

Edite `src/mqtt_client.c` função `is_server_network()`:

```c
// Verificar se está na rede do servidor (ex: 192.168.1.x)
return (ip_bytes[0] == 192 && ip_bytes[1] == 168 && ip_bytes[2] == 1);
```

## 🔧 Estrutura do Projeto

```
oledGPS/
├── include/
│   ├── pins.h          # Mapeamento de pinos
│   ├── oled.h          # Driver display OLED
│   ├── gps_parser.h    # Parser NMEA GPS
│   ├── wifi_http.h     # Servidor web
│   └── mqtt_client.h   # Cliente MQTT
├── src/
│   ├── main.c          # Aplicação principal
│   ├── oled.c          # Implementação OLED
│   ├── gps_parser.c    # Parser GPS
│   ├── wifi_http.c     # Servidor HTTP
│   └── mqtt_client.c   # Cliente MQTT
├── platformio.ini      # Configuração PlatformIO
└── README.md           # Este arquivo
```

## 🐛 Troubleshooting

### OLED não funciona

- Verifique conexões SDA/SCL (GPIO8/9)
- Confirme endereço I2C (0x3C ou 0x3D)
- Adicione pull-ups 4.7kΩ se necessário

### GPS sem sinal

- Verifique conexões RX/TX (GPIO20/21)
- Confirme alimentação 3.3V
- Aguarde TTFF (30-90s para primeiro fix)
- Teste com antena externa

### SD Card não monta

- Verifique conexões SPI (GPIO4-7)
- Confirme cartão FAT32
- Teste cartão em outro dispositivo

### MQTT não conecta

- Verifique IP do broker em `mqtt_client.h`
- Confirme rede do servidor em `is_server_network()`
- Teste conectividade: `ping 192.168.1.100`

## 📚 Referências

- [ESP32-C3 Super Mini Pinout](.snapshots/ESP32_C3_SUPER_MINI_PINOUT.md)
- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/)
- [PlatformIO ESP32-C3](https://docs.platformio.org/en/latest/boards/espressif32/esp32-c3-devkitm-1.html)

---
**Projeto**: OLEDGPS  
**Hardware**: ESP32-C3 Super Mini + GPS + OLED 128x64 + SD Card  
**Framework**: ESP-IDF via PlatformIO  
**Data**: Outubro 2025

