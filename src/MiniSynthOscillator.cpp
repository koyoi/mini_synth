#include "MiniSynthOscillator.h"

#include "MiniSynthMozziConfig.h"

#include <mozzi_pgmspace.h>
#include <tables/sin2048_int16.h>

namespace mini_synth {

OscWaveform analogToWaveform(const uint16_t value) {
  // 入力レンジを5等分して波形を選択。
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

int16_t sineFromTable(const uint16_t phase) {
  // 2048 エントリのテーブルに合わせて位相をスケーリング。
  const uint16_t index = phase >> 5U;
  return pgm_read_word_near(sin2048_int16 + index);
}

int16_t renderWave(const Voice &voice, const OscWaveform waveform) {
  // 位相を 16bit に正規化。
  const uint16_t phase = static_cast<uint16_t>(voice.phase >> 16U);
  switch (waveform) {
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

}  // namespace mini_synth

