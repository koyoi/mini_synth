// MiniSynthCpuLoad.cpp
#include "MiniSynthCpuLoad.h"

// Use micros() for high-resolution timing
static volatile uint32_t s_entryTime = 0; // last enter time (microseconds)
// Accumulated active time in microseconds since last sample
static volatile uint64_t s_activeAccum = 0;
// Number of audio callback invocations since last sample
static volatile uint32_t s_callCount = 0;

// Smoothed percent value
static float s_smoothedPercent = 0.0f;

void cpuLoadEnter() {
  // Record entry time
  s_entryTime = micros();
}

void cpuLoadExit() {
  // Compute elapsed since entry and accumulate
  const uint32_t now = micros();
  uint32_t entered = s_entryTime;
  // protect against wrap around of micros (which wraps ~ every 71 minutes)
  uint32_t delta = (now >= entered) ? (now - entered) : (UINT32_MAX - entered + now + 1);
  s_activeAccum += delta;
  ++s_callCount;
}

float cpuLoadSampleAndReset(uint32_t audioRate) {
  noInterrupts();
  const uint64_t activeUs = s_activeAccum;
  const uint32_t calls = s_callCount;
  s_activeAccum = 0;
  s_callCount = 0;
  interrupts();

  if (audioRate == 0 || calls == 0) {
    // nothing to report
    return s_smoothedPercent;
  }

  // Total period time in microseconds = calls * (1 / audioRate)
  // But we assume each call corresponds to one audio frame. For Mozzi, updateAudio
  // is called at kAudioRate. Use calls to calculate elapsed windows: elapsedUs = calls * (1e6 / audioRate)
  const double periodUs = static_cast<double>(calls) * (1000000.0 / static_cast<double>(audioRate));
  double pct = (static_cast<double>(activeUs) / periodUs) * 100.0;
  if (pct < 0.0) pct = 0.0;
  if (pct > 100.0) pct = 100.0;

  // Smooth
  s_smoothedPercent += CPU_LOAD_SMOOTH_ALPHA * (static_cast<float>(pct) - s_smoothedPercent);

#if defined(CPU_LOAD_DEBUG)
  Serial.print("[CPU LOAD] calls=");
  Serial.print(calls);
  Serial.print(" active(us)=");
  Serial.print(activeUs);
  Serial.print(" period(us)=");
  Serial.print(periodUs, 1);
  Serial.print(" pct=");
  Serial.print(pct, 2);
  Serial.print(" smooth=");
  Serial.println(s_smoothedPercent, 2);
#endif

  return s_smoothedPercent;
}

float cpuLoadGetLastPercent() {
  return s_smoothedPercent;
}
