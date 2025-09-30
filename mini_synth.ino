#include <Arduino.h>
#include <AudioConfig.h>
#include <MozziGuts.h>
#include <mozzi_midi.h>
#include <mozzi_pgmspace.h>
#include <tables/sin2048_int16.h>

/**
 * @brief 最大ボイス数。
 */
constexpr uint8_t kMaxVoices = 4U;

/**
 * @brief オーディオ更新レート。
 */
constexpr uint16_t kAudioRate = 16384U;

/**
 * @brief オーディオ設定オブジェクト。
 */
AudioConfig<kAudioRate> g_audioConfig;

/**
 * @brief コントロールレート。
 */
constexpr uint8_t kControlRate = 64U;

/**
 * @brief MIDI入力のシリアルポート。
 */
constexpr HardwareSerial &kMidiSerial = Serial1;

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
 * @brief エンベロープのステージ。
 */
enum class EnvelopeStage : uint8_t {
  kIdle = 0,
  kAttack,
  kSustain,
  kRelease,
};

/**
 * @brief MIDIメッセージの種別。
 */
enum class MidiMessage : uint8_t {
  kNoteOff = 0x80,
  kNoteOn = 0x90,
  kControlChange = 0xB0,
};

/**
 * @brief ボイス情報を保持する構造体。
 */
struct Voice {
  bool active = false;            //!< ボイスが使用中かどうか
  uint8_t note = 0U;              //!< 現在のMIDIノート番号
  uint32_t phase = 0U;            //!< 位相累積値（32bit固定小数）
  uint32_t increment = 0U;        //!< 現在の位相インクリメント
  uint32_t targetIncrement = 0U;  //!< ポルタメントで目標とするインクリメント
  int16_t envelope = 0;           //!< エンベロープ値（0-32767）
  EnvelopeStage stage = EnvelopeStage::kIdle; //!< エンベロープステージ
  uint8_t velocity = 0U;          //!< ベロシティ
  uint32_t age = 0U;              //!< 割り当て順を示すカウンタ
};

/**
 * @brief ポルタメントの平滑化係数（シフト量）。
 */
constexpr uint8_t kPortamentoShift = 4U;

/**
 * @brief 現在選択中の波形。
 */
volatile OscWaveform g_waveform = OscWaveform::kSine;

/**
 * @brief ボイス配列。
 */
Voice g_voices[kMaxVoices];

/**
 * @brief ボイス割り当て順序を管理するカウンタ。
 */
uint32_t g_voiceAgeCounter = 0U;

/**
 * @brief MIDI入力の状態保持用バッファ。
 */
uint8_t g_midiBuffer[3] = {0U};
uint8_t g_midiIndex = 0U;

/**
 * @brief ソフトポットの入力ピン定義。
 */
constexpr uint8_t kOscSelectPin = A0;
constexpr uint8_t kAttackPin = A1;
constexpr uint8_t kReleasePin = A2;
constexpr uint8_t kFilterPin = A3;
constexpr uint8_t kResonancePin = A4;

/**
 * @brief 5V入力を10bitに正規化するためのマクロ。
 */
constexpr uint16_t kAdcMax = 1023U;

/**
 * @brief アナログ値を波形に変換する。
 * @param value アナログ入力値。
 * @return 対応する波形種別。
 */
OscWaveform analogToWaveform(const uint16_t value) {
  // 値の範囲を5分割する。
  const uint16_t segment = kAdcMax / 5U;
  if (value < segment) {
    return OscWaveform::kSine;
  }
  if (value < segment * 2U) {
    return OscWaveform::kTriangle;
  }
  if (value < segment * 3U) {
    return OscWaveform::kSaw;
  }
  if (value < segment * 4U) {
    return OscWaveform::kPulse;
  }
  return OscWaveform::kSquare;
}

/**
 * @brief MIDIノート番号から位相インクリメント値を計算する。
 * @param note MIDIノート番号。
 * @return Mozzi向けの位相インクリメント値。
 */
uint32_t midiNoteToIncrement(const uint8_t note) {
  // Mozziの32bit位相インクリメント計算。
  const float frequency = mtof(note);
  const float scale = static_cast<float>(UINT32_MAX) / static_cast<float>(kAudioRate);
  return static_cast<uint32_t>(frequency * scale);
}

/**
 * @brief 空きボイスを検索する。
 * @return 使用可能なボイスへのポインタ。
 */
Voice *allocateVoice() {
  // まず非アクティブなボイスを探す。
  for (auto &voice : g_voices) {
    if (!voice.active) {
      return &voice;
    }
  }
  // 全ボイス使用中の場合は最も古いボイスを解放する。
  Voice *oldest = &g_voices[0];
  for (auto &voice : g_voices) {
    if (voice.age < oldest->age) {
      oldest = &voice;
    }
  }
  return oldest;
}

/**
 * @brief ボイスを初期化する。
 * @param voice 初期化対象のボイス。
 * @param note 設定するMIDIノート番号。
 * @param velocity 受信したベロシティ。
 */
void initVoice(Voice &voice, const uint8_t note, const uint8_t velocity) {
  // ボイス情報を初期化。
  voice.active = true;
  voice.note = note;
  voice.velocity = velocity;
  voice.phase = 0U;
  voice.targetIncrement = midiNoteToIncrement(note);
  voice.increment = voice.targetIncrement;
  voice.envelope = 0;
  voice.stage = EnvelopeStage::kAttack;
  voice.age = ++g_voiceAgeCounter;
}

/**
 * @brief 指定したノートに対応するボイスを探索する。
 * @param note 検索対象のMIDIノート番号。
 * @return 見つかったボイス、存在しなければnullptr。
 */
Voice *findVoiceByNote(const uint8_t note) {
  for (auto &voice : g_voices) {
    if (voice.active && voice.note == note) {
      return &voice;
    }
  }
  return nullptr;
}

/**
 * @brief ボイスのリリース処理を開始する。
 * @param voice 対象のボイス。
 */
void releaseVoice(Voice &voice) {
  // リリースステージに遷移。
  voice.stage = EnvelopeStage::kRelease;
}

/**
 * @brief ノートオンメッセージを処理する。
 * @param channel 受信チャンネル。
 * @param note ノート番号。
 * @param velocity ベロシティ。
 */
void noteOn(const uint8_t channel, const uint8_t note, const uint8_t velocity) {
  (void)channel;
  // 空きボイスに割り当てて初期化する。
  Voice *voice = allocateVoice();
  initVoice(*voice, note, velocity);
}

/**
 * @brief ノートオフメッセージを処理する。
 * @param channel 受信チャンネル。
 * @param note ノート番号。
 */
void noteOff(const uint8_t channel, const uint8_t note) {
  (void)channel;
  // 対応するボイスがあればリリース。
  Voice *voice = findVoiceByNote(note);
  if (voice != nullptr) {
    releaseVoice(*voice);
  }
}

/**
 * @brief MIDIメッセージをパースして処理する。
 * @param data 受信したMIDIバイト。
 */
void handleMidiByte(const uint8_t data) {
  // ステータスバイト判定。
  if ((data & 0x80U) != 0U) {
    g_midiBuffer[0] = data;
    g_midiIndex = 1U;
    return;
  }
  if (g_midiIndex == 0U) {
    return;
  }
  g_midiBuffer[g_midiIndex++] = data;
  // 完全なメッセージを構築したら処理する。
  const uint8_t status = g_midiBuffer[0] & 0xF0U;
  const uint8_t channel = g_midiBuffer[0] & 0x0FU;
  if (status == static_cast<uint8_t>(MidiMessage::kNoteOn) && g_midiIndex >= 3U) {
    if (g_midiBuffer[2] == 0U) {
      noteOff(channel, g_midiBuffer[1]);
    } else {
      noteOn(channel, g_midiBuffer[1], g_midiBuffer[2]);
    }
    g_midiIndex = 1U;
  } else if (status == static_cast<uint8_t>(MidiMessage::kNoteOff) && g_midiIndex >= 3U) {
    noteOff(channel, g_midiBuffer[1]);
    g_midiIndex = 1U;
  } else if (status == static_cast<uint8_t>(MidiMessage::kControlChange) && g_midiIndex >= 3U) {
    // コントロールチェンジは必要に応じて処理。
    g_midiIndex = 1U;
  }
}

/**
 * @brief ボイスのポルタメント処理を行う。
 * @param voice 対象のボイス。
 */
void updatePortamento(Voice &voice) {
  // 現在のインクリメントを固定小数点で平滑化。
  const int32_t current = static_cast<int32_t>(voice.increment);
  const int32_t target = static_cast<int32_t>(voice.targetIncrement);
  const int32_t diff = target - current;
  const int32_t step = diff >> kPortamentoShift;
  voice.increment = static_cast<uint32_t>(current + step);
}

/**
 * @brief サイン波テーブルから値を取得する。
 * @param phase 正規化済み位相値。
 * @return サイン波サンプル。
 */
int16_t sineFromTable(const uint16_t phase) {
  // 2048テーブルに合わせて位相を縮小。
  const uint16_t index = phase >> 5U;
  return pgm_read_word_near(sin2048_int16 + index);
}

/**
 * @brief 波形生成用の共通処理。
 * @param voice 波形生成対象のボイス。
 * @return 16bitの波形サンプル。
 */
int16_t renderWave(const Voice &voice) {
  // 位相上位ビットを抽出。
  const uint16_t phase = static_cast<uint16_t>(voice.phase >> 16U);
  switch (g_waveform) {
    case OscWaveform::kSine:
      // サイン波はテーブル参照で省リソース化。
      return sineFromTable(phase);
    case OscWaveform::kTriangle: {
      const int16_t tri = static_cast<int16_t>((phase < 32768U) ? (phase * 2) : (65535U - phase) * 2) - 32768;
      return tri;
    }
    case OscWaveform::kSaw:
      return static_cast<int16_t>((static_cast<int32_t>(phase) >> 1) - 32768);
    case OscWaveform::kPulse:
      return (phase < 32768U) ? 16384 : -16384;
    case OscWaveform::kSquare:
    default:
      return (phase < 32768U) ? 32767 : -32768;
  }
}

/**
 * @brief 音声サンプルを更新する。
 * @return モノラルのオーディオ出力。
 */
AudioOutput<MONO> updateAudio() {
  int32_t mix = 0;
  for (auto &voice : g_voices) {
    if (!voice.active) {
      continue;
    }
    const int16_t osc = renderWave(voice);
    // 固定小数点乗算で音量を適用。
    const int32_t sample = (static_cast<int32_t>(osc) * voice.envelope) >> 15;
    mix += sample;
    voice.phase += voice.increment;
  }
  mix = constrain(mix, -32768, 32767);
  return {static_cast<int16_t>(mix)};
}

/**
 * @brief エンベロープの更新処理。
 * @param voice 対象のボイス。
 * @param attackStep アタック時の増分。
 * @param releaseStep リリース時の減分。
 */
void updateEnvelope(Voice &voice, const int16_t attackStep, const int16_t releaseStep) {
  switch (voice.stage) {
    case EnvelopeStage::kAttack:
      if (voice.envelope + attackStep >= 32767) {
        voice.envelope = 32767;
        voice.stage = EnvelopeStage::kSustain;
      } else {
        voice.envelope = voice.envelope + attackStep;
      }
      break;
    case EnvelopeStage::kSustain:
      // サステイン中は値を保持。
      break;
    case EnvelopeStage::kRelease:
      if (voice.envelope <= releaseStep) {
        voice.envelope = 0;
        voice.stage = EnvelopeStage::kIdle;
        voice.active = false;
      } else {
        voice.envelope = voice.envelope - releaseStep;
      }
      break;
    case EnvelopeStage::kIdle:
    default:
      break;
  }
}

/**
 * @brief エンベロープとポルタメントを更新する。
 */
void updateVoices() {
  const int16_t attackStep = static_cast<int16_t>(map(analogRead(kAttackPin), 0, kAdcMax, 256, 4096));
  const int16_t releaseStep = static_cast<int16_t>(map(analogRead(kReleasePin), 0, kAdcMax, 128, 2048));
  for (auto &voice : g_voices) {
    if (!voice.active) {
      continue;
    }
    updateEnvelope(voice, attackStep, releaseStep);
    updatePortamento(voice);
  }
}

/**
 * @brief コントロール入力の処理。
 */
void updateControl() {
  // ポットの値から波形とエンベロープを調整。
  g_waveform = analogToWaveform(analogRead(kOscSelectPin));
  updateVoices();
  // シリアルMIDIを読み出して処理。
  while (kMidiSerial.available() > 0) {
    const uint8_t data = static_cast<uint8_t>(kMidiSerial.read());
    handleMidiByte(data);
  }
}

/**
 * @brief 初期化処理。
 */
void setup() {
  // アナログ入力を有効化。
  pinMode(kOscSelectPin, INPUT);
  pinMode(kAttackPin, INPUT);
  pinMode(kReleasePin, INPUT);
  pinMode(kFilterPin, INPUT);
  pinMode(kResonancePin, INPUT);
  kMidiSerial.begin(31250);
  // Mozzi2のオーディオ設定を適用。
  g_audioConfig.apply();
  startMozzi(kControlRate);
}

/**
 * @brief メインループ。
 */
void loop() {
  audioHook();
}

