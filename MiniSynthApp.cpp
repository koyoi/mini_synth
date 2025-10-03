#include <arduino.h>
#include <MozziHeadersOnly.h>

#include "MiniSynthApp.h"

#include "MiniSynthMidi.h"
#include "MiniSynthOscillator.h"
#include "MiniSynthVoice.h"
#include "MiniSynthCpuLoad.h"
#include "MiniSynthScope.h"
#include "MiniSynthDisplay.h"

namespace mini_synth {
namespace {
/**
 * @brief シンセサイザー全体の状態を保持する。
 */
SynthState g_state;

// グローバル SVF 状態（GLOBAL_SVF が有効な場合に使用）
float g_filter_low = 0.0f;
float g_filter_band = 0.0f;
float g_global_f = 0.0f; // 正規化周波数
float g_global_q = 0.0f; // レゾナンス（0..1）
// audio-rate smoothing state
float g_global_f_smooth = 0.0f;
float g_global_q_smooth = 0.0f;

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

AudioOutput generateAudio() {
  int32_t mix = 0;
  for (auto &voice : g_state.voices) {
    if (!voice.active) {
      continue;
    }
    // 選択された波形を生成。
    const int16_t osc = renderWave(voice, g_state.waveform);
    // エンベロープ値を適用して振幅を調整。
    const int32_t sample = (static_cast<int32_t>(osc) * voice.envelope) >> 15;
#if VOICE_SVF
    // per-voice SVF が有効な場合はボイスごとにフィルタ処理を行う。
    float s = static_cast<float>(sample);
  // キー追従のため、プリコンピュートしたテーブルを参照
  const uint8_t noteIdx = (voice.note <= 127) ? voice.note : 127;
  const float f = kNoteFTable[noteIdx];
    const float q = g_global_q; // レゾナンスはグローバルノブで共有
    s = processVoiceSVF(voice, s, f, q);
    mix += static_cast<int32_t>(s);
#else
    mix += sample;
#endif
    // 次回サンプル用に位相を進める。
    voice.phase += voice.increment;
  }
  // 出力レンジに収める。
  mix = constrain(mix, -32768, 32767);
  // グローバル SVF が有効な場合はミックス後に SVF を適用する
#if GLOBAL_SVF
  // 入力を float に正規化
  float in = static_cast<float>(mix);
  // audio-rate smoothing (simple 1-pole)
  const float alpha = 0.2f; // smoothing factor (tuneable)
  g_global_f_smooth += alpha * (g_global_f - g_global_f_smooth);
  g_global_q_smooth += alpha * (g_global_q - g_global_q_smooth);
  const float f = g_global_f_smooth;
  const float q = g_global_q_smooth;
  // 高域 (hp) を計算
  float hp = in - g_filter_low - q * g_filter_band;
  g_filter_band += f * hp;
  g_filter_low += f * g_filter_band;
  float out = g_filter_low; // ローパス出力
  // クリップして戻す
  // soft clip (tanh-like) to avoid harsh clipping and tame oscillation
  const float clipA = 1.0f / 32768.0f;
  float x = out * clipA;
  // simple soft clip: x / (1 + |x|)
  float y = x / (1.0f + fabsf(x));
  out = y / clipA;
  out = constrain(out, -32768.0f, 32767.0f);
  return {static_cast<int16_t>(out)};
#else
  return {static_cast<int16_t>(mix)};
#endif
}

void handleControl() {
  // 波形選択ポットの値を読み取り、波形を更新。
  g_state.waveform = analogToWaveform(analogRead(kOscSelectPin));
  // エンベロープパラメータを計算。
  const int16_t attackStep = static_cast<int16_t>(map(analogRead(kAttackPin), 0, kAdcMax, 256, 4096));
  const int16_t releaseStep = static_cast<int16_t>(map(analogRead(kReleasePin), 0, kAdcMax, 128, 2048));
  // フィルタ関連を読み取る
  const uint16_t rawCut = analogRead(kFilterPin);
  const uint16_t rawRes = analogRead(kResonancePin);
  // カットオフは指数マップで自然な応答にする（80Hz..6000Hz を例）
  const float fc = 80.0f * powf(6000.0f / 80.0f, static_cast<float>(rawCut) / static_cast<float>(kAdcMax));
  // 正規化 f を簡易計算（f = 2 * sin(pi * fc / fs) 相当のスケール）
  g_global_f = 2.0f * sinf(M_PI * fc / static_cast<float>(kAudioRate));
  // レゾナンスは 0..0.95 程度でクリップ
  g_global_q = constrain(static_cast<float>(rawRes) / static_cast<float>(kAdcMax), 0.0f, 0.95f);
  updateActiveVoices(attackStep, releaseStep);
  // MIDI データを読み出し、必要なイベントを処理。
  while (midiSerial().available() > 0) {
    const uint8_t data = static_cast<uint8_t>(midiSerial().read());
    handleMidiByte(g_state, data);
  }

  // 簡易鍵盤スキャン（ポーリング）。押している間ノートを保持し、離すとノートオフ。
  // 現在は 5 鍵の直接 GPIO 実装。
  const uint8_t keyPins[5] = {kKeyPin0, kKeyPin1, kKeyPin2, kKeyPin3, kKeyPin4};
  for (uint8_t i = 0; i < 5; ++i) {
    const bool pressed = (digitalRead(keyPins[i]) == LOW); // pullup 想定
    // 探して既に鳴いているボイスをチェック
    Voice *v = findVoiceByNote(g_state, kKeyNotes[i]);
    if (pressed) {
      if (v == nullptr) {
        // ノートオン
        noteOn(g_state, 0, kKeyNotes[i], 127);
      }
    } else {
      if (v != nullptr) {
        // ノートオフ
        noteOff(g_state, 0, kKeyNotes[i]);
      }
    }
  }

  // control-rate で CPU 使用率をサンプリングしてリセット
  // Mozzi の control rate は kControlRate (定義は MiniSynthMozziConfig.h)
  float cpuPct = cpuLoadSampleAndReset(kAudioRate);
#if defined(CPU_LOAD_DEBUG)
  // ユーザーがデバッグを有効にした場合はシリアルに出す（Serial.begin は initializeSynth で必要）
  Serial.print("CPU %: ");
  Serial.println(cpuPct, 1);
#endif

  // Waveform / Spectrum display: take a snapshot of recent samples
  const size_t dispN = 128; // number of samples to display / analyze
  static int16_t snap[dispN];
  scopeSnapshot(snap, dispN);
  // update waveform on display (stubbed if display not enabled)
  displayUpdateWaveform(snap, dispN);

  // very small DFT (real-input) to compute magnitudes for low bins
  const size_t bins = 32; // display bins
  static float mag[bins];
  for (size_t k = 0; k < bins; ++k) {
    float real = 0.0f, imag = 0.0f;
    for (size_t n = 0; n < dispN; ++n) {
      const float angle = -2.0f * M_PI * static_cast<float>(k) * static_cast<float>(n) / static_cast<float>(dispN);
      const float s = static_cast<float>(snap[n]) / 32768.0f;
      real += s * cosf(angle);
      imag += s * sinf(angle);
    }
    mag[k] = sqrtf(real * real + imag * imag);
  }
  displayUpdateSpectrum(mag, bins);
}

void initializeSynth() {
  // アナログ入力ピンの初期化。
  pinMode(kOscSelectPin, INPUT);
  pinMode(kAttackPin, INPUT);
  pinMode(kReleasePin, INPUT);
  pinMode(kFilterPin, INPUT);
  pinMode(kResonancePin, INPUT);
  // デジタル鍵盤ピンをプルアップで初期化
  pinMode(kKeyPin0, INPUT_PULLUP);
  pinMode(kKeyPin1, INPUT_PULLUP);
  pinMode(kKeyPin2, INPUT_PULLUP);
  pinMode(kKeyPin3, INPUT_PULLUP);
  pinMode(kKeyPin4, INPUT_PULLUP);
  // MIDI シリアルを初期化。
  midiSerial().begin(31250);
  // Mozzi のオーディオ処理を開始。
  startMozzi(kControlRate);
  // display init (stub if disabled)
  displayInit();
}

}  // namespace mini_synth

