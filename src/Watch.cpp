#include "Watch.h"

uint16_t COL_BG;
uint16_t COL_CYAN;
uint16_t COL_CYAN_DIM;
uint16_t COL_MAGENTA;
uint16_t COL_MAGENTA_DIM;

Watch::Watch(Adafruit_ST7789& tft) : tft(tft) {
    COL_BG          = ST77XX_BLACK;
    COL_CYAN        = tft.color565(0, 255, 255);
    COL_CYAN_DIM    = tft.color565(0, 55, 65);
    COL_MAGENTA     = tft.color565(255, 0, 170);
    COL_MAGENTA_DIM = tft.color565(65, 0, 50);
}

Watch::~Watch() {}

void Watch::drawSyncPulse(bool pulseOn) {
    int cx = SCREEN_W - 26;
    int cy = 10;
    tft.fillCircle(cx, cy, 2, pulseOn ? COL_CYAN : COL_CYAN_DIM);
}

void Watch::glowRect(int x, int y, int w, int h, uint16_t bright, uint16_t dim) {
    tft.fillRect(x - 1, y - 1, w + 2, h + 2, dim);
    tft.fillRect(x, y, w, h, bright);
}

void Watch::drawDigit(int x, int y, int digit, uint16_t bright, uint16_t dim) {
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

void Watch::drawColon(int x, bool visible) {
    int dotX = x + (COLON_SLOT - SEG_THICK) / 2;
    if (visible) {
        glowRect(dotX, TIME_Y + DIGIT_H / 3 - SEG_THICK / 2, SEG_THICK, SEG_THICK, COL_CYAN, COL_CYAN_DIM);
        glowRect(dotX, TIME_Y + 2 * DIGIT_H / 3 - SEG_THICK / 2, SEG_THICK, SEG_THICK, COL_CYAN, COL_CYAN_DIM);
    } else {
        tft.fillRect(dotX - 1, TIME_Y + DIGIT_H / 3 - SEG_THICK / 2 - 1, SEG_THICK + 2, SEG_THICK + 2, COL_BG);
        tft.fillRect(dotX - 1, TIME_Y + 2 * DIGIT_H / 3 - SEG_THICK / 2 - 1, SEG_THICK + 2, SEG_THICK + 2, COL_BG);
    }
}

void Watch::drawTime(const struct tm& timeinfo) {
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

void Watch::drawDate(const String& weekday, const String& month, const struct tm& timeinfo) {
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

void Watch::drawFrame() {
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

void Watch::drawCustomFrame() {
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