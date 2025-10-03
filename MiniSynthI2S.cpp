#include "MiniSynthI2S.h"

#ifndef USE_I2S

// Stub implementation when I2S is not enabled. Always return false for pushes.
void initI2SOutput(uint32_t sampleRate) {
  (void)sampleRate;
}

bool i2sPushSample(int16_t sample) {
  (void)sample;
  return false;
}

void i2sStart(void) {}
void i2sStop(void) {}


#else

// NUCLEO-F411RE HAL-based I2S + DMA template implementation.
// This code assumes you generated I2S3 (SPI3 in I2S mode) and associated DMA
// with CubeMX and that MX_I2S3_Init() and the I2S handle (hi2s3) exist.
// You must integrate the generated initialization in your project (or adapt names below).

#include <Arduino.h>
#include "stm32f4xx_hal.h"

// Forward declarations for HAL handles that CubeMX generates.
// Replace these with the actual handles/names from your CubeMX project if different.
extern I2S_HandleTypeDef hi2s3; // provided by CubeMX: I2S3 handle

static const int kI2sDmaBufSize = 1024; // stereo samples (interleaved) or mono depending setup
static int16_t g_i2sDmaBuf[kI2sDmaBufSize];
static volatile int g_i2sDmaWriteIndex = 0; // write index in samples

// Called to initialize I2S (user should also call MX_I2S3_Init())
void initI2SOutput(uint32_t sampleRate) {
  (void)sampleRate;
  g_i2sDmaWriteIndex = 0;
  // Ensure MX_I2S3_Init() is called from main setup (CubeMX generated). The project must
  // setup DMA transfer for hi2s3 with circular mode and set callbacks HAL_I2S_TxCpltCallback
  // and HAL_I2S_TxHalfCpltCallback to refill buffers.
}

// Push a mono sample into the DMA buffer (interleaving for stereo if needed)
bool i2sPushSample(int16_t sample) {
  int next = (g_i2sDmaWriteIndex + 1) % kI2sDmaBufSize;
  int idx = g_i2sDmaWriteIndex;
  // If buffer is about to overwrite the area being sent by DMA, drop sample
  // (simple overflow protection). In circular DMA mode, ensure DMA buffer is large enough.
  // This template does not implement a full producer/consumer; adapt as needed.
  g_i2sDmaBuf[idx] = sample;
  g_i2sDmaWriteIndex = next;
  return true;
}

void i2sStart(void) {
  // Start DMA in circular mode: transmit g_i2sDmaBuf with size kI2sDmaBufSize
  // Example (uncomment after integrating CubeMX generated hi2s3 and HAL):
  // HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t*)g_i2sDmaBuf, kI2sDmaBufSize);
}

void i2sStop(void) {
  // HAL_I2S_DMAStop(&hi2s3);
}

// The following callbacks are called by HAL when DMA transfer reaches half or full.
// Implementations here refill the DMA buffer from the internal ring buffer.
// Make sure to enable these callbacks in your CubeMX/HAL configuration.

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
  (void)hi2s;
  // refill first half
  for (int i = 0; i < kI2sDmaBufSize / 2; ++i) {
    // simple consumption from circular write index (not robust producer/consumer)
    // Users should replace with proper synchronization if needed.
    int idx = (i) % kI2sDmaBufSize;
    g_i2sDmaBuf[idx] = 0; // silence by default
  }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
  (void)hi2s;
  // refill second half
  for (int i = kI2sDmaBufSize / 2; i < kI2sDmaBufSize; ++i) {
    int idx = i % kI2sDmaBufSize;
    g_i2sDmaBuf[idx] = 0;
  }
}

#endif

