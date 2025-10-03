#pragma once
#include <Arduino.h>

// Define ENABLE_DISPLAY to include display support (requires U8g2)
// #define ENABLE_DISPLAY 1

void displayInit();
void displayUpdateWaveform(const int16_t *samples, size_t n);
void displayUpdateSpectrum(const float *mag, size_t n);
