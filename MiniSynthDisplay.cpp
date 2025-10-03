#include "MiniSynthDisplay.h"

#if defined(ENABLE_DISPLAY)
#include <U8g2lib.h>

// Example: SSD1309 SPI constructor (user must adjust pins)
// U8G2_SSD1309_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
U8G2 u8g2(U8G2_R0);

void displayInit() {
  // Initialize U8g2 (user must set correct constructor)
  u8g2.begin();
}

void displayUpdateWaveform(const int16_t *samples, size_t n) {
  if (!n) return;
  u8g2.clearBuffer();
  const int w = u8g2.getDisplayWidth();
  const int h = u8g2.getDisplayHeight();
  // draw waveform scaled to display
  for (size_t i = 0; i < n - 1; ++i) {
    int x1 = (i * w) / n;
    int x2 = (((i + 1) * w) / n);
    int y1 = h/2 - ((samples[i] * (h/2)) / 32768);
    int y2 = h/2 - ((samples[i+1] * (h/2)) / 32768);
    u8g2.drawLine(x1, y1, x2, y2);
  }
  u8g2.sendBuffer();
}

// Very small FFT magnitude display (caller computes mag)
void displayUpdateSpectrum(const float *mag, size_t n) {
  if (!n) return;
  u8g2.clearBuffer();
  const int w = u8g2.getDisplayWidth();
  const int h = u8g2.getDisplayHeight();
  const size_t bins = n;
  for (size_t i = 0; i < bins; ++i) {
    int x = (i * w) / bins;
    int hbar = (int)(mag[i] * (float)h);
    if (hbar > h) hbar = h;
    u8g2.drawVLine(x, h - hbar, 1);
  }
  u8g2.sendBuffer();
}

#else

void displayInit() {
  // stub
}
void displayUpdateWaveform(const int16_t *samples, size_t n) {
  (void)samples; (void)n;
}
void displayUpdateSpectrum(const float *mag, size_t n) {
  (void)mag; (void)n;
}

#endif
