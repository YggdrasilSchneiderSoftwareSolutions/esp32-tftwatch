#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <esp_sntp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "secrets.h"
#include "Watch.h"

// Auskommentieren, wenn keine Debug-Ausgaben stattfinden sollen (Produktion)
#define DEBUG

// WIFI Einstellungen
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Zeitzone für Deutschland mit automatischer Sommer-/Winterzeit
const char* ntpServer = "pool.ntp.org";
const char* timezone = "CET-1CEST,M3.5.0/02,M10.5.0/03";
// Wie oft die interne Uhr per NTP synchronisiert werden soll (in Stunden)
// dazwischen läuft die Uhrzeit über die Chipt-Zeitbasis weiter, kein Netzwerkzugriff nötig
constexpr uint32_t NTP_SYNC_INTERVAL_HOURS = 6 * 60UL * 60UL * 1000UL; // 6 Stunden in ms
 
// Display
// Dedizierte SPI pins nutzen
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

Watch watch(tft);
 
void setup() {
    Serial.begin(115200);
  
    // Hintergrundbeleuchtung einschalten
    pinMode(TFT_BACKLITE, OUTPUT);
    digitalWrite(TFT_BACKLITE, HIGH);
  
    // TFT / I2C power supply einschalten
    pinMode(TFT_I2C_POWER, OUTPUT);
    digitalWrite(TFT_I2C_POWER, HIGH);
    delay(10);
  
    // Display initialisieren
    tft.init(135, 240); // Init ST7789 240x135
    tft.setRotation(3);
  
    tft.fillScreen(COL_BG);
    tft.setTextWrap(false);
  
    watch.drawFrame();
  
    Serial.println(F("Display initialisiert, verbinde mit WLAN..."));
  
    // WLAN verbinden
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println(F("\nWLAN verbunden!"));
  
    // Sync-Intervall setzen
    sntp_set_sync_interval(NTP_SYNC_INTERVAL_HOURS);
    // NTP-Zeit initialisieren
    configTzTime(timezone, ntpServer);
}
 
void loop() {
    watch.drawCustomFrame();
}