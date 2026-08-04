#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <esp_sntp.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "secrets.h"

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

uint32_t UPDATE_INTERVAL_MS = 1000; // Update-Intervall für die Anzeige (1 Sekunde)
uint32_t lastUpdateTime = 0; // Zeitstempel der letzten Anzeigeaktualisierung

// Deutsche Texte für die Anzeige
const String de_weekdays[] = {
  "Sonntag",
  "Montag",
  "Dienstag",
  "Mittwoch",
  "Donnerstag",
  "Freitag",
  "Samstag",
};
const String de_months[] = {
  "Januar",
  "Februar",
  "März",
  "April",
  "Mai",
  "Juni",
  "Juli",
  "August",
  "September",
  "Oktober",
  "November",
  "Dezember"
};
 
// Display
// Dedizierte SPI pins nutzen
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
 
// ---- Cyberpunk HUD palette ----
uint16_t COL_BG;
uint16_t COL_CYAN;
uint16_t COL_CYAN_DIM;
uint16_t COL_MAGENTA;
uint16_t COL_MAGENTA_DIM;
 
// ---- Screen geometry (240x135, rotation 3) ----
const int SCREEN_W = 240;
const int SCREEN_H = 135;
 
// ---- Seven-segment digit geometry ----
const int DIGIT_W   = 26;
const int DIGIT_H   = 46;
const int SEG_THICK = 5;
const int COLON_SLOT = 14;
const int SLOT_GAP   = 4;
 
const int TIME_ROW_WIDTH = 6 * DIGIT_W + 2 * COLON_SLOT + 7 * SLOT_GAP; // = 212
const int TIME_X = (SCREEN_W - TIME_ROW_WIDTH) / 2;                     // = 14
const int TIME_Y = 30;
 
const int DIVIDER_Y1 = 18;
const int DIVIDER_Y2 = 84;
const int DATE_Y     = 94;
const int FOOTER_Y   = 116;
 
// segments per digit: A(top), B(top-right), C(bottom-right), D(bottom), E(bottom-left), F(top-left), G(middle)
const bool digitSegments[10][7] = {
  {1,1,1,1,1,1,0}, // 0
  {0,1,1,0,0,0,0}, // 1
  {1,1,0,1,1,0,1}, // 2
  {1,1,1,1,0,0,1}, // 3
  {0,1,1,0,0,1,1}, // 4
  {1,0,1,1,0,1,1}, // 5
  {1,0,1,1,1,1,1}, // 6
  {1,1,1,0,0,0,0}, // 7
  {1,1,1,1,1,1,1}, // 8
  {1,1,1,1,0,1,1}, // 9
};
 
// Draws a filled rect with a 1px dim halo behind it, to fake a neon glow on the TFT.
void glowRect(int x, int y, int w, int h, uint16_t bright, uint16_t dim) {
  tft.fillRect(x - 1, y - 1, w + 2, h + 2, dim);
  tft.fillRect(x, y, w, h, bright);
}
 
void drawDigit(int x, int y, int digit, uint16_t bright, uint16_t dim) {
  const bool* seg = digitSegments[digit];
  int halfH = DIGIT_H / 2;
 
  if (seg[0]) glowRect(x + SEG_THICK, y, DIGIT_W - 2 * SEG_THICK, SEG_THICK, bright, dim);                        // A
  if (seg[1]) glowRect(x + DIGIT_W - SEG_THICK, y, SEG_THICK, halfH, bright, dim);                                // B
  if (seg[2]) glowRect(x + DIGIT_W - SEG_THICK, y + halfH, SEG_THICK, halfH, bright, dim);                        // C
  if (seg[3]) glowRect(x + SEG_THICK, y + DIGIT_H - SEG_THICK, DIGIT_W - 2 * SEG_THICK, SEG_THICK, bright, dim);  // D
  if (seg[4]) glowRect(x, y + halfH, SEG_THICK, halfH, bright, dim);                                              // E
  if (seg[5]) glowRect(x, y, SEG_THICK, halfH, bright, dim);                                                      // F
  if (seg[6]) glowRect(x + SEG_THICK, y + halfH - SEG_THICK / 2, DIGIT_W - 2 * SEG_THICK, SEG_THICK, bright, dim);// G
}
 
void drawColon(int x, bool visible) {
  int dotX = x + (COLON_SLOT - SEG_THICK) / 2;
  if (visible) {
    glowRect(dotX, TIME_Y + DIGIT_H / 3 - SEG_THICK / 2, SEG_THICK, SEG_THICK, COL_CYAN, COL_CYAN_DIM);
    glowRect(dotX, TIME_Y + 2 * DIGIT_H / 3 - SEG_THICK / 2, SEG_THICK, SEG_THICK, COL_CYAN, COL_CYAN_DIM);
  } else {
    tft.fillRect(dotX - 1, TIME_Y + DIGIT_H / 3 - SEG_THICK / 2 - 1, SEG_THICK + 2, SEG_THICK + 2, COL_BG);
    tft.fillRect(dotX - 1, TIME_Y + 2 * DIGIT_H / 3 - SEG_THICK / 2 - 1, SEG_THICK + 2, SEG_THICK + 2, COL_BG);
  }
}
 
void drawTime(const struct tm& timeinfo) {
  char buf[7];
  snprintf(buf, sizeof(buf), "%02d%02d%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
 
  tft.fillRect(TIME_X, TIME_Y - 1, TIME_ROW_WIDTH, DIGIT_H + 2, COL_BG);
 
  const char pattern[] = { 'D', 'D', ':', 'D', 'D', ':', 'D', 'D' };
  int x = TIME_X;
  int digitIdx = 0;
  bool colonVisible = true; // Falls Doppelpunke blinken sollen: (timeinfo.tm_sec % 2) == 0;
 
  for (char token : pattern) {
    if (token == 'D') {
      drawDigit(x, TIME_Y, buf[digitIdx] - '0', COL_CYAN, COL_CYAN_DIM);
      digitIdx++;
      x += DIGIT_W + SLOT_GAP;
    } else {
      drawColon(x, colonVisible);
      x += COLON_SLOT + SLOT_GAP;
    }
  }
}
 
void drawDate(const String& weekday, const String& month, const struct tm& timeinfo) {
  String weekdayUpper = weekday;
  String monthUpper = month;
  weekdayUpper.toUpperCase();
  monthUpper.toUpperCase();
 
  String dateLine = weekdayUpper + " / " + String(timeinfo.tm_mday) + "." + monthUpper + "." + String(1900 + timeinfo.tm_year);
 
  int16_t x1, y1;
  uint16_t w, h;
  tft.setTextSize(1);
  tft.getTextBounds(dateLine, 0, 0, &x1, &y1, &w, &h);
 
  tft.fillRect(TIME_X, DATE_Y, TIME_ROW_WIDTH, h + 2, COL_BG);
  tft.setCursor(TIME_X + (TIME_ROW_WIDTH - w) / 2, DATE_Y);
  tft.setTextColor(COL_MAGENTA);
  tft.print(dateLine);
}
 
void drawSyncPulse(bool pulseOn) {
  int cx = SCREEN_W - 26;
  int cy = 10;
  tft.fillCircle(cx, cy, 2, pulseOn ? COL_CYAN : COL_CYAN_DIM);
}
 
// Static HUD chrome that never changes: corner brackets, header/footer labels, divider lines.
void drawFrame() {
  const int armLen = 14;
  const int thick = 2;
  const int m = 4;
 
  // top-left
  tft.fillRect(m, m, armLen, thick, COL_CYAN);
  tft.fillRect(m, m, thick, armLen, COL_CYAN);
  // top-right
  tft.fillRect(SCREEN_W - m - armLen, m, armLen, thick, COL_CYAN);
  tft.fillRect(SCREEN_W - m - thick, m, thick, armLen, COL_CYAN);
  // bottom-left
  tft.fillRect(m, SCREEN_H - m - thick, armLen, thick, COL_CYAN);
  tft.fillRect(m, SCREEN_H - m - armLen, thick, armLen, COL_CYAN);
  // bottom-right
  tft.fillRect(SCREEN_W - m - armLen, SCREEN_H - m - thick, armLen, thick, COL_CYAN);
  tft.fillRect(SCREEN_W - m - thick, SCREEN_H - m - armLen, thick, armLen, COL_CYAN);
 
  tft.setTextSize(1);
  tft.setTextColor(COL_CYAN_DIM);
  tft.setCursor(m + armLen + 4, 6);
  tft.print("CHRONO-OS");
  tft.setCursor(SCREEN_W - m - armLen - 34, 6);
  tft.print("SYNC");
 
  tft.fillRect(TIME_X, DIVIDER_Y1, TIME_ROW_WIDTH, 1, COL_CYAN_DIM);
  tft.fillRect(TIME_X, DIVIDER_Y2, TIME_ROW_WIDTH, 1, COL_MAGENTA_DIM);
 
  String footer = "// LOCALE: DE  TZ: CET-CEST //";
  int16_t x1, y1;
  uint16_t w, h;
  tft.getTextBounds(footer, 0, 0, &x1, &y1, &w, &h);
  tft.setTextColor(COL_MAGENTA_DIM);
  tft.setCursor((SCREEN_W - w) / 2, FOOTER_Y);
  tft.print(footer);
}
 
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
 
  COL_BG          = ST77XX_BLACK;
  COL_CYAN        = tft.color565(0, 255, 255);
  COL_CYAN_DIM    = tft.color565(0, 55, 65);
  COL_MAGENTA     = tft.color565(255, 0, 170);
  COL_MAGENTA_DIM = tft.color565(65, 0, 50);
 
  tft.fillScreen(COL_BG);
  tft.setTextWrap(false);
  drawFrame();
 
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
  // Display Update nur alle UPDATE_INTERVAL_MS Millisekunden (besser als delay)
  if (millis() - lastUpdateTime >= UPDATE_INTERVAL_MS) {
    lastUpdateTime = millis();
  
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      // Wochentag und Monat übersetzen
      String weekday = de_weekdays[timeinfo.tm_wday];
      String month = de_months[timeinfo.tm_mon];
      #ifdef DEBUG
      Serial.println(&timeinfo, "%A, %d %B %Y %H:%M:%S");
      #endif
    
      drawTime(timeinfo);

      // Standardfont Uhrzeit gross in Cyan, Trenner in Magenta
      /*
      const int16_t CLOCK_Y = 32;
      const int16_t CLOCK_CLR_X = 44, CLOCK_CLR_Y = 32, CLOCK_CLR_W = 152, CLOCK_CLR_H = 26;
      uint16_t COL_CLOCK     = tft.color565(0, 255, 255);
      uint16_t COL_CLOCK_SEP = tft.color565(255, 0, 200);
      char hh[3], mm[3], ss[3];
      sprintf(hh, "%02d", timeinfo.tm_hour);
      sprintf(mm, "%02d", timeinfo.tm_min);
      sprintf(ss, "%02d", timeinfo.tm_sec);
      tft.fillRect(CLOCK_CLR_X, CLOCK_CLR_Y, CLOCK_CLR_W, CLOCK_CLR_H, COL_BG);
      tft.setTextWrap(false);
      tft.setTextSize(4);
      tft.setCursor(CLOCK_CLR_X, CLOCK_Y);
      tft.setTextColor(COL_CLOCK, COL_BG);     tft.print(hh);
      tft.setTextColor(COL_CLOCK_SEP, COL_BG); tft.print(":");
      tft.setTextColor(COL_CLOCK, COL_BG);     tft.print(mm);
      tft.setTextColor(COL_CLOCK_SEP, COL_BG); tft.print(":");
      tft.setTextColor(COL_CLOCK, COL_BG);     tft.print(ss);
      */

      drawDate(weekday, month, timeinfo);
      drawSyncPulse(timeinfo.tm_sec % 2 == 0);
    } else {
      drawSyncPulse(false);
      Serial.println(F("Warte auf Zeitsynchronisation..."));
    }
  }
}