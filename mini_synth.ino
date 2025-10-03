#include "src/MiniSynthMozziConfig.h"
#include <Mozzi.h>

#include "src/MiniSynthApp.h"

/**
 * @brief Arduino 初期化ルーチン。
 */
void setup() {
  mini_synth::initializeSynth();
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
AudioOutput<MONO> updateAudio() {
  return mini_synth::generateAudio();
}

/**
 * @brief Arduino メインループ。
 */
void loop() {
  audioHook();
}

