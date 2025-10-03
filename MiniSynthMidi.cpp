#include <arduino.h>
#include <MozziHeadersOnly.h>

#include "MiniSynthMidi.h"

#include "MiniSynthMozziConfig.h"

namespace mini_synth {

void noteOn(SynthState &state, const uint8_t channel, const uint8_t note, const uint8_t velocity) {
  (void)channel;
  // 空きボイスを割り当てて初期化。
  Voice *voice = allocateVoice(state);
  if (voice != nullptr) {
    initVoice(state, *voice, note, velocity);
  }
}

void noteOff(SynthState &state, const uint8_t channel, const uint8_t note) {
  (void)channel;
  // 対応するボイスを探索し、リリースを開始。
  Voice *voice = findVoiceByNote(state, note);
  if (voice != nullptr) {
    releaseVoice(*voice);
  }
}

void handleMidiByte(SynthState &state, const uint8_t data) {
  // ステータスバイトの場合はバッファをリセット。
  if ((data & 0x80U) != 0U) {
    state.midi.buffer[0] = data;
    state.midi.index = 1U;
    return;
  }
  if (state.midi.index == 0U) {
    // ステータスが未受信の場合は無視。
    return;
  }
  // データバイトをバッファへ追加。
  state.midi.buffer[state.midi.index++] = data;
  const uint8_t status = state.midi.buffer[0] & 0xF0U;
  const uint8_t channel = state.midi.buffer[0] & 0x0FU;
  if (status == static_cast<uint8_t>(MidiMessage::kNoteOn) && state.midi.index >= 3U) {
    if (state.midi.buffer[2] == 0U) {
      noteOff(state, channel, state.midi.buffer[1]);
    } else {
      noteOn(state, channel, state.midi.buffer[1], state.midi.buffer[2]);
    }
    state.midi.index = 1U;
  } else if (status == static_cast<uint8_t>(MidiMessage::kNoteOff) && state.midi.index >= 3U) {
    noteOff(state, channel, state.midi.buffer[1]);
    state.midi.index = 1U;
  } else if (status == static_cast<uint8_t>(MidiMessage::kControlChange) && state.midi.index >= 3U) {
    // 今回はコントロールチェンジを利用しないため読み飛ばす。
    state.midi.index = 1U;
  }
}

}  // namespace mini_synth

