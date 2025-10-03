#include "MiniSynthMozziConfig.h"
#include <Mozzi.h>

#include "MiniSynthApp.h"

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
AudioOutput updateAudio() {
  return mini_synth::generateAudio();
}

/**
 * @brief Arduino メインループ。
 */
void loop() {
  audioHook();
}

