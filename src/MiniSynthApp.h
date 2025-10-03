#pragma once

#include "MiniSynthMozziConfig.h"

#include <AudioOutput.h>

#include "MiniSynthTypes.h"

namespace mini_synth {

/**
 * @brief シンセサイザーの初期化処理を実行する。
 */
void initializeSynth();

/**
 * @brief コントロール更新処理を行う。
 */
void handleControl();

/**
 * @brief 現在の状態からオーディオサンプルを生成する。
 * @return モノラルオーディオ出力。
 */
AudioOutput<MONO> generateAudio();

}  // namespace mini_synth

