# ESP32-C3 Super Mini - Pinout Reference

**Referência oficial:** <https://www.espboards.dev/esp32/esp32-c3-super-mini/>

## 📋 Especificações

- **MCU**: ESP32-C3FH4 (QFN32, RISC-V 32-bit, single core @ 160MHz)
- **Flash**: 4MB embutida (XMC)
- **RAM**: 400KB SRAM
- **WiFi**: 2.4GHz 802.11 b/g/n
- **Bluetooth**: BLE 5.0
- **USB**: USB-C com USB-Serial/JTAG integrado
- **Tamanho**: 22.52 x 18mm

## 🔌 Pinout Completo

### Pinos Digitais Disponíveis

| GPIO | Funções Alternativas         | Notas                           |
|------|------------------------------|---------------------------------|
| 0    | ADC1_CH0, XTAL_32K_P         | Disponível                      |
| 1    | ADC1_CH1, XTAL_32K_N         | Disponível                      |
| 2    | ADC1_CH2, FSPIQ              | Disponível                      |
| 3    | ADC1_CH3, FSPIHD             | Disponível                      |
| 4    | ADC1_CH4, FSPICS0, MTMS      | SD_CS (SPI)                     |
| 5    | ADC2_CH0, FSPIWP             | SD_MISO (SPI)                   |
| 6    | FSPICLK, MTCK                | SD_SCK (SPI)                    |
| 7    | FSPID, MTDO                  | SD_MOSI (SPI)                   |
| 8    | **LED_BUILTIN**, I2C_SDA     | OLED SDA (I2C)                  |
| 9    | I2C_SCL                      | OLED SCL (I2C)                  |
| 10   | FSPICS0                      | Disponível                      |
| 20   | U0RXD (USB)                  | GPS RX / USB-Serial (conflito)  |
| 21   | U0TXD (USB)                  | GPS TX / USB-Serial (conflito)  |

### Pinos de Alimentação

| Pino | Função         | Tensão     |
|------|----------------|------------|
| 5V   | USB Power In   | 5V         |
| 3V3  | Regulador Out  | 3.3V       |
| GND  | Ground         | 0V         |

## ⚠️ Notas Importantes

### LED Interno (GPIO8)

- O ESP32-C3 Super Mini possui um **LED azul interno** conectado ao **GPIO8**
- No projeto atual, o GPIO8 é usado como **I2C SDA do OLED**
- O LED foi **REMOVIDO** do firmware para liberar o GPIO8
- Não utilize GPIO8 para outras funções

### I2C (Display OLED)

- **SDA**: GPIO8 (OLED)
- **SCL**: GPIO9 (OLED)
- Pode ser compartilhado com sensores adicionais compatíveis com I2C

### USB-Serial Integrado

- **NÃO** requer chip externo (CP2102/CH340)
- GPIO20/21 automaticamente usados para USB
- Programação e debug via USB-C direto

### Boot/Reset

- **Botão BOOT** : GPIO9 (pull-up interno)
- **Botão RESET**: EN pin
- Para entrar em modo download: BOOT pressionado + RESET

## 🎯 Configuração do Projeto (OLED + SD + GPS)

### Pinos Utilizados

```c
// OLED 128x64 I2C (SSD1306/SH1106)
#define OLED_SDA_GPIO   8   // I2C SDA
#define OLED_SCL_GPIO   9   // I2C SCL

// SD Card via SPI
#define SD_CS_GPIO      4   // Chip Select
#define SD_SCK_GPIO     6   // SPI Clock
#define SD_MOSI_GPIO    7   // SPI MOSI
#define SD_MISO_GPIO    5   // SPI MISO

// GPS UART (NMEA)
#define GPS_RX_GPIO     20  // ESP RXD (conectado ao TX do GPS)
#define GPS_TX_GPIO     21  // ESP TXD (conectado ao RX do GPS)
```

### Pinos Disponíveis para Expansão

- **GPIO0-3, GPIO10**: Livres para uso geral (ver conflitos com ADC e FSPI)
- **GPIO8-9**: Em uso para I2C do display
- **GPIO4-7**: Em uso para SPI do SD card
- **GPIO20-21**: Em uso para GPS (UART0); evitam-se durante programação USB
- **ADC**: Pinos 0-5 podem ser usados como ADC se necessário (respeitar usos atuais)

### Avisos Importantes (GPS em GPIO20/21)

- GPIO20/21 são mapeados como **U0RXD/U0TXD (USB-Serial)**. Usá-los para GPS pode conflitar com programação e logs via USB.
- Recomenda-se desconectar o GPS ao programar via USB-C, ou mover o GPS para outro UART/pinos caso necessário.

## 📚 Referências

1. **Pinout Oficial**: <https://www.espboards.dev/esp32/esp32-c3-super-mini/#onboardLed>
2. **Datasheet ESP32-C3**: <https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf>
3. **ESP-IDF Programming Guide**: <https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/>

## 🔧 Hardware Notes

### Regulador de Tensão

- **ME6211C33M5G**: LDO 3.3V, 500mA máximo
- Entrada: 5V via USB-C
- Não ultrapassar 500mA no 3V3 pin

### Flash

- **XMC XM25QH32B**: 4MB SPI Flash, 80MHz
- Suporta OTA updates
- 2MB usados por padrão (partition table)

---

**Atualizado:** 14 de outubro de 2025  
**Projeto:** OLEDGPS
**Observação:** LED interno GPIO8 removido do firmware para liberar I2C
