#include "MiniSynthMozziConfig.h"
#include <Mozzi.h>

#include "MiniSynthApp.h"
#include "MiniSynthI2S.h"
#include "MiniSynthCpuLoad.h"
#include "MiniSynthScope.h"
#include "MiniSynthDisplay.h"

/**
 * @brief Arduino 初期化ルーチン。
 */
void setup() {
  mini_synth::initializeSynth();
#ifdef USE_I2S
  // Initialize I2S output at Mozzi audio rate
  initI2SOutput(MOZZI_AUDIO_RATE);
  i2sStart();
#endif
}

/**
 * @brief Mozzi のコントロール処理フック。
 */
void updateControl() {
  mini_synth::handleControl();
}

/**
 * @brief Mozzi のオーディオ生成フック。
 * @return モノラルオーディオ出力。
 */
AudioOutput updateAudio() {
  cpuLoadEnter();
  auto out = mini_synth::generateAudio();
  // push sample to scope buffer for visualization
  scopePushSample(out.output);
#ifdef USE_I2S
  // push sample to I2S (if enabled). If push fails, fallback to Mozzi output
  if (!i2sPushSample(out.output)) {
    cpuLoadExit();
    return out;
  }
  // Still return Mozzi's expected output for compatibility (unused when I2S driving external DAC)
  cpuLoadExit();
  return out;
#else
  cpuLoadExit();
  return out;
#endif
}

/**
 * @brief Arduino メインループ。
 */
void loop() {
  audioHook();
}

