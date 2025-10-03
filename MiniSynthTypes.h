#pragma once

#include <Arduino.h>

namespace mini_synth {

/**
 * @brief 使用する最大ボイス数。
 */
constexpr uint8_t kMaxVoices = 4U;

/**
 * @brief オーディオサンプルレート。
 */
constexpr uint16_t kAudioRate = 16384U;

/**
 * @brief コントロールレート。
 */
constexpr uint8_t kControlRate = 64U;

/**
 * @brief ポルタメントの平滑係数（シフト量）。
 */
constexpr uint8_t kPortamentoShift = 4U;

/**
 * @brief オシレータ選択用のアナログ入力ピン。
 */
constexpr uint8_t kOscSelectPin = A0;

/**
 * @brief アタックパラメータのアナログ入力ピン。
 */
constexpr uint8_t kAttackPin = A1;

/**
 * @brief リリースパラメータのアナログ入力ピン。
 */
constexpr uint8_t kReleasePin = A2;

/**
 * @brief フィルタカットオフ用のアナログ入力ピン。
 */
constexpr uint8_t kFilterPin = A3;

/**
 * @brief フィルタレゾナンス用のアナログ入力ピン。
 */
constexpr uint8_t kResonancePin = A4;

/**
 * @brief デジタル入力（鍵盤）ピンの定義。
 *
 * 現在は 5 鍵を直接 GPIO から読み取る簡易実装です。将来的にはダイオード付きマトリクススキャンに置き換えます。
 */
constexpr uint8_t kKeyPin0 = 2;
constexpr uint8_t kKeyPin1 = 3;
constexpr uint8_t kKeyPin2 = 4;
constexpr uint8_t kKeyPin3 = 5;
constexpr uint8_t kKeyPin4 = 6;

// キーに割り当てる MIDI ノート番号（C, E, G, A, D の例）
static const uint8_t kKeyNotes[5] = {60, 64, 67, 69, 62};

/**
 * @brief 10bit ADC の最大値。
 */
constexpr uint16_t kAdcMax = 1023U;

/**
 * @brief オシレータ波形の種類。
 */
enum class OscWaveform : uint8_t {
  kSine = 0,
  kTriangle,
  kSaw,
  kPulse,
  kSquare,
};

/**
 * @brief エンベロープの各ステージ。
 */
enum class EnvelopeStage : uint8_t {
  kIdle = 0,
  kAttack,
  kSustain,
  kRelease,
};

/**
 * @brief MIDI メッセージの種別。
 */
enum class MidiMessage : uint8_t {
  kNoteOff = 0x80,
  kNoteOn = 0x90,
  kControlChange = 0xB0,
};

/**
 * @brief 単一ボイスの状態を保持する構造体。
 */
struct Voice {
  bool active = false;                 //!< ボイスが有効かどうか。
  uint8_t note = 0U;                   //!< 割り当てられている MIDI ノート番号。
  uint32_t phase = 0U;                 //!< 位相値（固定小数点32bit）。
  uint32_t increment = 0U;             //!< 現在の位相インクリメント。
  uint32_t targetIncrement = 0U;       //!< ポルタメントの目標インクリメント。
  int16_t envelope = 0;                //!< エンベロープ値。
  EnvelopeStage stage = EnvelopeStage::kIdle; //!< 現在のエンベロープステージ。
  uint8_t velocity = 0U;               //!< 受信ベロシティ。
  uint32_t age = 0U;                   //!< 割り当て順序を識別するカウンタ。
  // SVF 用の軽量状態（VOICE_SVF 使用時に利用）
  float svf_low = 0.0f;                //!< SVF ロー出力状態
  float svf_band = 0.0f;               //!< SVF バンド出力状態
};

/**
 * @brief MIDI 解析に使用するワークバッファ。
 */
struct MidiParser {
  uint8_t buffer[3] = {0U}; //!< 受信データの保持領域。
  uint8_t index = 0U;       //!< 現在格納中のバイト数。
};

/**
 * @brief シンセ全体の状態をまとめたコンテナ。
 */
struct SynthState {
  Voice voices[kMaxVoices];               //!< 利用可能なボイス群。
  uint32_t voiceAgeCounter = 0U;          //!< 次に割り当てるボイス年齢。
  volatile OscWaveform waveform = OscWaveform::kSine; //!< 現在選択中の波形。
  MidiParser midi;                        //!< MIDI パーサ状態。
};

/**
 * @brief MIDI 入力に使用するシリアルポートへの参照を取得する。
 * @return MIDI 用の HardwareSerial 参照。
 */
inline HardwareSerial &midiSerial() {
  return Serial1;
}

// ビルド時に以下のマクロでフィルタ方式を切り替えできます。
// 定義例:
// -DGLOBAL_SVF : ミックス後にグローバルな SVF を適用（デフォルト）
// -DVOICE_SVF  : 各ボイスごとに SVF を持ち、キー追従でカットオフを変化させる
#ifndef GLOBAL_SVF
#define GLOBAL_SVF 1
#endif


}  // namespace mini_synth

