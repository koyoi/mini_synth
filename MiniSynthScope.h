#pragma once
#include <Arduino.h>

// Simple lock-free circular buffer for last N audio samples.
// Usage:
//  - call scopePushSample(sample) from audio callback (ISR)
//  - call scopeSnapshot(outBuf, n) from control-rate to copy most recent n samples

#ifndef SCOPE_BUFFER_SIZE
#define SCOPE_BUFFER_SIZE 256
#endif

void scopePushSample(int16_t sample);
/**
 * Copy the most recent n samples into outBuf (n <= SCOPE_BUFFER_SIZE).
 * The samples are ordered oldest..newest in the buffer.
 */
void scopeSnapshot(int16_t *outBuf, size_t n);
