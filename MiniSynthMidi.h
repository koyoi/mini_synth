#pragma once

#include "MiniSynthTypes.h"
#include "MiniSynthVoice.h"

namespace mini_synth {

/**
 * @brief ノートオンメッセージを処理する。
 * @param state シンセ状態。
 * @param channel 受信チャンネル。
 * @param note ノート番号。
 * @param velocity ベロシティ値。
 */
void noteOn(SynthState &state, uint8_t channel, uint8_t note, uint8_t velocity);

/**
 * @brief ノートオフメッセージを処理する。
 * @param state シンセ状態。
 * @param channel 受信チャンネル。
 * @param note ノート番号。
 */
void noteOff(SynthState &state, uint8_t channel, uint8_t note);

/**
 * @brief MIDI バイト列を解析して処理する。
 * @param state シンセ状態。
 * @param data 受信した MIDI データバイト。
 */
void handleMidiByte(SynthState &state, uint8_t data);

}  // namespace mini_synth

