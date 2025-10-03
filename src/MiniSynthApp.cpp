#include "MiniSynthApp.h"

#include <Mozzi.h>

#include "MiniSynthMidi.h"
#include "MiniSynthOscillator.h"
#include "MiniSynthVoice.h"

namespace mini_synth {
namespace {
/**
 * @brief シンセサイザー全体の状態を保持する。
 */
SynthState g_state;

/**
 * @brief エンベロープとポルタメントを更新するユーティリティ。
 * @param attackStep アタック時の増分。
 * @param releaseStep リリース時の減分。
 */
void updateActiveVoices(const int16_t attackStep, const int16_t releaseStep) {
  for (auto &voice : g_state.voices) {
    if (!voice.active) {
      continue;
    }
    // エンベロープ更新とポルタメント適用をそれぞれ実行。
    updateEnvelope(voice, attackStep, releaseStep);
    updatePortamento(voice);
  }
}
}  // namespace

AudioOutput<MONO> generateAudio() {
  int32_t mix = 0;
  for (auto &voice : g_state.voices) {
    if (!voice.active) {
      continue;
    }
    // 選択された波形を生成。
    const int16_t osc = renderWave(voice, g_state.waveform);
    // エンベロープ値を適用して振幅を調整。
    const int32_t sample = (static_cast<int32_t>(osc) * voice.envelope) >> 15;
    mix += sample;
    // 次回サンプル用に位相を進める。
    voice.phase += voice.increment;
  }
  // 出力レンジに収める。
  mix = constrain(mix, -32768, 32767);
  return {static_cast<int16_t>(mix)};
}

void handleControl() {
  // 波形選択ポットの値を読み取り、波形を更新。
  g_state.waveform = analogToWaveform(analogRead(kOscSelectPin));
  // エンベロープパラメータを計算。
  const int16_t attackStep = static_cast<int16_t>(map(analogRead(kAttackPin), 0, kAdcMax, 256, 4096));
  const int16_t releaseStep = static_cast<int16_t>(map(analogRead(kReleasePin), 0, kAdcMax, 128, 2048));
  updateActiveVoices(attackStep, releaseStep);
  // MIDI データを読み出し、必要なイベントを処理。
  while (midiSerial().available() > 0) {
    const uint8_t data = static_cast<uint8_t>(midiSerial().read());
    handleMidiByte(g_state, data);
  }
}

void initializeSynth() {
  // アナログ入力ピンの初期化。
  pinMode(kOscSelectPin, INPUT);
  pinMode(kAttackPin, INPUT);
  pinMode(kReleasePin, INPUT);
  pinMode(kFilterPin, INPUT);
  pinMode(kResonancePin, INPUT);
  // MIDI シリアルを初期化。
  midiSerial().begin(31250);
  // Mozzi のオーディオ処理を開始。
  startMozzi(kControlRate);
}

}  // namespace mini_synth

