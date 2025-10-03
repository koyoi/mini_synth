#include "MiniSynthScope.h"

static volatile int16_t s_buffer[SCOPE_BUFFER_SIZE];
static volatile uint16_t s_index = 0;

void scopePushSample(int16_t sample) {
  // Called from ISR; store and advance index atomically (wrap)
  s_buffer[s_index] = sample;
  ++s_index;
  if (s_index >= SCOPE_BUFFER_SIZE) s_index = 0;
}

void scopeSnapshot(int16_t *outBuf, size_t n) {
  if (n == 0 || n > SCOPE_BUFFER_SIZE) return;
  // Disable interrupts while copying to ensure consistent snapshot
  noInterrupts();
  uint16_t idx = s_index; // next write index
  // oldest sample is at idx (since idx points to next free)
  uint16_t start = (idx >= n) ? (idx - n) : (SCOPE_BUFFER_SIZE + idx - n);
  for (size_t i = 0; i < n; ++i) {
    uint16_t pos = start + i;
    if (pos >= SCOPE_BUFFER_SIZE) pos -= SCOPE_BUFFER_SIZE;
    outBuf[i] = s_buffer[pos];
  }
  interrupts();
}
