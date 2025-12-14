/*
 * ESP8266 NodeMCU GPS Tracker com OLED (Opcional)
 * 
 * Conexões:
 * GPS Module (NEO-6M/7M):
 *   - GPS TX -> D6 (GPIO12)
 *   - GPS RX -> D5 (GPIO14) 
 *   - VCC -> 3.3V
 *   - GND -> GND
 * 
 * OLED Display SSD1306 (Opcional):
 *   - SDA -> D2 (GPIO4)
 *   - SCL -> D1 (GPIO5)
 *   - VCC -> 3.3V
 *   - GND -> GND
 */

#include <SoftwareSerial.h>
#include <TinyGPSPlus.h>

// Configuração do display OLED (opcional)
#define USE_OLED false  // Mude para true se tiver OLED conectado
#if USE_OLED
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  #define SCREEN_WIDTH 128
  #define SCREEN_HEIGHT 64
  #define OLED_RESET -1
  Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#endif

// Pinos GPS
#define GPS_RX_PIN D6  // GPIO12 - recebe do GPS TX
#define GPS_TX_PIN D5  // GPIO14 - envia para GPS RX

// Objetos GPS
SoftwareSerial gpsSerial(GPS_RX_PIN, GPS_TX_PIN);
TinyGPSPlus gps;

// Variáveis de controle
unsigned long lastDisplayUpdate = 0;
unsigned long lastSerialUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 1000;  // Atualizar display a cada 1s
const unsigned long SERIAL_INTERVAL = 2000;   // Atualizar serial a cada 2s

void setup() {
  // Inicializar Serial para debug
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("ESP8266 GPS Tracker");
  Serial.println("===================");
  
  // Inicializar GPS
  gpsSerial.begin(9600);
  Serial.println("GPS inicializado em 9600 baud");
  Serial.print("RX: D6 (GPIO"); Serial.print(GPS_RX_PIN); Serial.println(")");
  Serial.print("TX: D5 (GPIO"); Serial.print(GPS_TX_PIN); Serial.println(")");
  
  #if USE_OLED
    // Inicializar OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println("OLED não encontrado!");
    } else {
      Serial.println("OLED inicializado");
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      display.println("GPS Tracker");
      display.println("Iniciando...");
      display.display();
    }
  #endif
  
  Serial.println();
  Serial.println("Aguardando sinal GPS...");
  Serial.println("(Pode levar alguns minutos em ambiente interno)");
}

void loop() {
  // Ler dados do GPS
  while (gpsSerial.available() > 0) {
    char c = gpsSerial.read();
    gps.encode(c);
    
    // Debug raw NMEA (opcional - descomente para ver)
    // Serial.write(c);
  }
  
  unsigned long now = millis();
  
  // Atualizar Serial Monitor
  if (now - lastSerialUpdate >= SERIAL_INTERVAL) {
    lastSerialUpdate = now;
    printGPSData();
  }
  
  #if USE_OLED
    // Atualizar Display OLED
    if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
      lastDisplayUpdate = now;
      updateDisplay();
    }
  #endif
  
  // Verificar timeout do GPS
  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println("AVISO: Nenhum dado GPS detectado!");
    Serial.println("Verifique as conexões:");
    Serial.println("  - GPS TX -> D6 (GPIO12)");
    Serial.println("  - GPS RX -> D5 (GPIO14)");
    Serial.println("  - Alimentação GPS: 3.3V");
    delay(5000);
  }
}

void printGPSData() {
  Serial.println("\n--- Dados GPS ---");
  
  // Localização
  if (gps.location.isValid()) {
    Serial.print("Latitude : ");
    Serial.println(gps.location.lat(), 8);
    Serial.print("Longitude: ");
    Serial.println(gps.location.lng(), 8);
    Serial.print("Link Maps: https://www.google.com/maps?q=");
    Serial.print(gps.location.lat(), 6);
    Serial.print(",");
    Serial.println(gps.location.lng(), 6);
  } else {
    Serial.println("Localização: AGUARDANDO FIX");
  }
  
  // Satélites
  if (gps.satellites.isValid()) {
    Serial.print("Satélites: ");
    Serial.println(gps.satellites.value());
  } else {
    Serial.println("Satélites: N/A");
  }
  
  // Altitude
  if (gps.altitude.isValid()) {
    Serial.print("Altitude : ");
    Serial.print(gps.altitude.meters());
    Serial.println(" m");
  }
  
  // Velocidade
  if (gps.speed.isValid()) {
    Serial.print("Velocidade: ");
    Serial.print(gps.speed.kmph());
    Serial.println(" km/h");
  }
  
  // Data e Hora
  if (gps.date.isValid() && gps.time.isValid()) {
    Serial.print("Data/Hora: ");
    if (gps.date.day() < 10) Serial.print("0");
    Serial.print(gps.date.day());
    Serial.print("/");
    if (gps.date.month() < 10) Serial.print("0");
    Serial.print(gps.date.month());
    Serial.print("/");
    Serial.print(gps.date.year());
    Serial.print(" ");
    if (gps.time.hour() < 10) Serial.print("0");
    Serial.print(gps.time.hour());
    Serial.print(":");
    if (gps.time.minute() < 10) Serial.print("0");
    Serial.print(gps.time.minute());
    Serial.print(":");
    if (gps.time.second() < 10) Serial.print("0");
    Serial.print(gps.time.second());
    Serial.println(" UTC");
  }
  
  // Estatísticas
  Serial.print("Caracteres processados: ");
  Serial.println(gps.charsProcessed());
  Serial.print("Sentenças válidas: ");
  Serial.println(gps.sentencesWithFix());
  Serial.print("Sentenças falhas: ");
  Serial.println(gps.failedChecksum());
  
  Serial.println("----------------");
}

#if USE_OLED
void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  
  // Título
  display.println("GPS Tracker");
  display.println("-------------");
  
  if (gps.location.isValid()) {
    // Localização
    display.print("LAT: ");
    display.println(gps.location.lat(), 6);
    display.print("LNG: ");
    display.println(gps.location.lng(), 6);
    
    // Satélites e velocidade
    if (gps.satellites.isValid()) {
      display.print("SAT: ");
      display.print(gps.satellites.value());
    }
    if (gps.speed.isValid()) {
      display.print(" SPD: ");
      display.print((int)gps.speed.kmph());
      display.println("km/h");
    }
    
    // Altitude
    if (gps.altitude.isValid()) {
      display.print("ALT: ");
      display.print((int)gps.altitude.meters());
      display.println("m");
    }
    
  } else {
    display.println("Aguardando GPS...");
    display.println();
    if (gps.satellites.isValid()) {
      display.print("Satelites: ");
      display.println(gps.satellites.value());
    }
    display.println();
    display.println("Pode levar alguns");
    display.println("minutos...");
  }
  
  display.display();
}
#endif
