#pragma once

#include <Adafruit_ST7789.h>

// ---- Cyberpunk HUD palette ----
extern uint16_t COL_BG;
extern uint16_t COL_CYAN;
extern uint16_t COL_CYAN_DIM;
extern uint16_t COL_MAGENTA;
extern uint16_t COL_MAGENTA_DIM;
 
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

class Watch {
    private:
        const uint32_t UPDATE_INTERVAL_MS = 1000; // Update-Intervall für die Anzeige (1 Sekunde)
        uint32_t lastUpdateTime = 0; // Zeitstempel der letzten Anzeigeaktualisierung

        // Draws a filled rect with a 1px dim halo behind it, to fake a neon glow on the TFT.
        void glowRect(int x, int y, int w, int h, uint16_t bright, uint16_t dim);
        void drawDigit(int x, int y, int digit, uint16_t bright, uint16_t dim);
        void drawColon(int x, bool visible);
        void drawTime(const struct tm& timeinfo);
        void drawDate(const String& weekday, const String& month, const struct tm& timeinfo);
    
    protected:
        Adafruit_ST7789& tft;

    public:
        Watch(Adafruit_ST7789& tft);
        virtual ~Watch();

        void drawSyncPulse(bool pulseOn = false);
        // Static HUD chrome that never changes: corner brackets, header/footer labels, divider lines.
        void drawFrame();
        /* virtual method to override */
        virtual void drawCustomFrame();
};