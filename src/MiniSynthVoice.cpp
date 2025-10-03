#include "MiniSynthVoice.h"

#include "MiniSynthMozziConfig.h"

#include <mozzi_midi.h>

namespace mini_synth {

uint32_t midiNoteToIncrement(const uint8_t note) {
  // MIDI ノート番号を周波数に変換。
  const float frequency = mtof(note);
  // 周波数から固定小数点の位相インクリメントへ変換するスケールを算出。
  const float scale = static_cast<float>(UINT32_MAX) / static_cast<float>(kAudioRate);
  return static_cast<uint32_t>(frequency * scale);
}

Voice *allocateVoice(SynthState &state) {
  // まず非アクティブなボイスを探索。
  for (auto &voice : state.voices) {
    if (!voice.active) {
      return &voice;
    }
  }
  // すべて使用中の場合は最も age の小さいボイスを再利用する。
  Voice *oldest = &state.voices[0];
  for (auto &voice : state.voices) {
    if (voice.age < oldest->age) {
      oldest = &voice;
    }
  }
  return oldest;
}

void initVoice(SynthState &state, Voice &voice, const uint8_t note, const uint8_t velocity) {
  // 新しいノート情報でボイスを再初期化。
  voice.active = true;
  voice.note = note;
  voice.velocity = velocity;
  voice.phase = 0U;
  voice.targetIncrement = midiNoteToIncrement(note);
  voice.increment = voice.targetIncrement;
  voice.envelope = 0;
  voice.stage = EnvelopeStage::kAttack;
  // age カウンタを更新し、LRU 判定に備える。
  voice.age = ++state.voiceAgeCounter;
}

Voice *findVoiceByNote(SynthState &state, const uint8_t note) {
  // 同じノート番号を持つアクティブなボイスを探す。
  for (auto &voice : state.voices) {
    if (voice.active && voice.note == note) {
      return &voice;
    }
  }
  return nullptr;
}

void releaseVoice(Voice &voice) {
  // リリースフェーズに遷移し、エンベロープ減衰を開始。
  voice.stage = EnvelopeStage::kRelease;
}

void updatePortamento(Voice &voice) {
  // 現在値と目標値の差分を計算。
  const int32_t current = static_cast<int32_t>(voice.increment);
  const int32_t target = static_cast<int32_t>(voice.targetIncrement);
  const int32_t diff = target - current;
  // シフト演算による簡易一次 IIR で平滑化。
  const int32_t step = diff >> kPortamentoShift;
  voice.increment = static_cast<uint32_t>(current + step);
}

void updateEnvelope(Voice &voice, const int16_t attackStep, const int16_t releaseStep) {
  switch (voice.stage) {
    case EnvelopeStage::kAttack:
      // アタック中は指定ステップで増加させる。
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
      // 指定ステップで減少させ、ゼロに到達したらボイスを無効化。
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
      // アイドル状態では何もしない。
      break;
  }
}

}  // namespace mini_synth

