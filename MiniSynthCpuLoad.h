// MiniSynthCpuLoad.h
#pragma once
#include <Arduino.h>

/**
 * Lightweight CPU load measurement for Mozzi audio callback.
 *
 * Usage:
 *  - Call cpuLoadEnter() at start of audio callback (ISR).
 *  - Call cpuLoadExit() at end of audio callback (ISR).
 *  - Periodically (control rate) call cpuLoadSampleAndReset(kAudioRate) to
 *    compute the percent CPU load since the last sample and reset accumulators.
 *
 * Build-time options:
 *  - Define CPU_LOAD_DEBUG to enable periodic Serial prints (requires Serial.begin).
 */

// Enable periodic Serial debug output when defined
//#define CPU_LOAD_DEBUG 1

// Smoothing factor applied when computing reported percent (0..1). 0 = no smoothing.
#ifndef CPU_LOAD_SMOOTH_ALPHA
#define CPU_LOAD_SMOOTH_ALPHA 0.2f
#endif

void cpuLoadEnter();
void cpuLoadExit();

/**
 * Compute CPU load percent since last sample (and reset accumulators).
 * @param audioRate Audio callback rate in Hz (e.g. kAudioRate)
 * @return Smoothed CPU load percent (0..100)
 */
float cpuLoadSampleAndReset(uint32_t audioRate);

/**
 * Get the last computed CPU load percent without resetting.
 */
float cpuLoadGetLastPercent();
