#pragma once

#include <stdint.h>

// Simple I2S output interface used by the synth. Default a stub is provided.
// Implement platform-specific I2S/DMA code and replace the stub when enabling USE_I2S.

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize I2S output at given sample rate (Hz).
 */
void initI2SOutput(uint32_t sampleRate);

/**
 * @brief Push one signed 16-bit sample into the I2S output queue.
 * @return true if sample was queued, false if dropped (queue full or not enabled).
 */
bool i2sPushSample(int16_t sample);

/**
 * @brief Start I2S output (enable DMA etc.).
 */
void i2sStart(void);

/**
 * @brief Stop I2S output.
 */
void i2sStop(void);

#ifdef __cplusplus
}
#endif
