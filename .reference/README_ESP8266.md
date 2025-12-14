# ESP8266 NodeMCU GPS Tracker

## ⚠️ PROBLEMA ATUAL: ESP8266 não detectado

O ESP8266 NodeMCU não está sendo reconhecido pelo Windows. Possíveis causas:

### 1. Driver CH340 não instalado
O NodeMCU usa chip CH340G para comunicação USB-Serial.

**Solução:**
- Baixe o driver em: https://sparks.gogo.co.nz/ch340.html
- Ou busque "CH340 driver Windows" no Google
- Instale e reinicie o computador

### 2. Cabo USB inadequado
Alguns cabos são apenas para alimentação (sem dados).

**Solução:**
- Use um cabo USB com **4 fios** (dados + alimentação)
- Teste em outra porta USB

### 3. ESP8266 em modo bootloader
Às vezes o chip fica travado.

**Solução:**
- Desconecte o USB
- Segure o botão **FLASH** pressionado
- Conecte o USB novamente
- Solte o botão FLASH
- Tente detectar novamente

## 🔌 Diagrama de Conexões

```
ESP8266 NodeMCU          GPS Module (NEO-6M/7M)
┌─────────────┐          ┌──────────────┐
│             │          │              │
│    3.3V ────┼──────────┼─── VCC       │
│     GND ────┼──────────┼─── GND       │
│      D4 ────┼──────────┼─── TX (GPS)  │
│      D3 ────┼──────────┼─── RX (GPS)  │
│             │          │              │
└─────────────┘          └──────────────┘

OLED Display SSD1306 (OPCIONAL)
┌──────────────┐
│              │
│   VCC ───────┼─── 3.3V (ESP8266)
│   GND ───────┼─── GND
│   SDA ───────┼─── D2 (GPIO4)
│   SCL ───────┼─── D1 (GPIO5)
│              │
└──────────────┘
```

## 📦 Como usar (quando detectado)

### 1. Instalar dependências
```bash
pio lib install
```

### 2. Compilar para ESP8266
```bash
pio run -e nodemcu
```

### 3. Fazer upload
```bash
pio run -e nodemcu -t upload
```

### 4. Monitorar Serial
```bash
pio device monitor -b 115200
```

## 📝 Configurações do Código

No arquivo `esp8266_gps.ino`:

```cpp
#define USE_OLED false  // Mude para true se tiver OLED
```

## 🛠️ Verificar Porta COM

Depois de instalar o driver, execute:

```bash
pio device list
```

Ou em Python:
```bash
python -c "import serial.tools.list_ports; [print(f'{p.device} - {p.description}') for p in serial.tools.list_ports.comports()]"
```

## 📊 Saída Esperada

Quando funcionar, você verá no Serial Monitor:

```
ESP8266 GPS Tracker
===================
GPS inicializado em 9600 baud
Aguardando sinal GPS...

--- Dados GPS ---
Latitude : -23.5505199
Longitude: -46.6333094
Link Maps: https://www.google.com/maps?q=-23.550520,-46.633309
Satélites: 8
Altitude : 760 m
Velocidade: 0.0 km/h
Data/Hora: 22/11/2025 14:32:15 UTC
Caracteres processados: 1542
Sentenças válidas: 12
Sentenças falhas: 0
```

## ⏱️ Tempo de Fix GPS

- **Ambiente externo:** 30 segundos a 2 minutos
- **Perto de janela:** 2 a 5 minutos
- **Ambiente interno:** Pode não funcionar (precisa ver o céu)

## 🔗 Links Úteis

- [Pinout NodeMCU ESP8266](https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/)
- [Biblioteca TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus)
- [Driver CH340](https://sparks.gogo.co.nz/ch340.html)

## 🆚 Diferenças ESP32-C3 vs ESP8266

| Recurso | ESP32-C3 | ESP8266 |
|---------|----------|---------|
| Framework | ESP-IDF | Arduino |
| CPU | RISC-V 160MHz | Xtensa 80MHz |
| RAM | 400KB | 80KB |
| WiFi | 802.11 b/g/n | 802.11 b/g/n |
| Bluetooth | BLE 5.0 | ❌ Não |
| Pinos GPIO | Mais flexíveis | Limitados |
| UART Hardware | 2 | 1.5 (GPS via Software) |

---

**Próximos passos:**
1. ✅ Código criado (`esp8266_gps.ino`)
2. ✅ Configuração PlatformIO (`platformio.ini`)
3. ⏳ **Aguardando detecção do ESP8266**
4. ⏳ Fazer upload e testar
