#pragma once

#include "MiniSynthTypes.h"

namespace mini_synth {

/**
 * @brief MIDI ノート番号から位相インクリメントを計算する。
 * @param note 対象の MIDI ノート番号。
 * @return 固定小数点の位相インクリメント値。
 */
uint32_t midiNoteToIncrement(uint8_t note);

/**
 * @brief 利用可能なボイスを取得する。
 * @param state シンセ状態。
 * @return 割り当て可能なボイスへのポインタ。
 */
Voice *allocateVoice(SynthState &state);

/**
 * @brief ボイス情報を初期化する。
 * @param state シンセ状態。
 * @param voice 初期化対象のボイス。
 * @param note 割り当てるノート番号。
 * @param velocity 受信ベロシティ。
 */
void initVoice(SynthState &state, Voice &voice, uint8_t note, uint8_t velocity);

/**
 * @brief 指定したノートに対応するボイスを検索する。
 * @param state シンセ状態。
 * @param note 検索するノート番号。
 * @return 見つかったボイス、存在しない場合は nullptr。
 */
Voice *findVoiceByNote(SynthState &state, uint8_t note);

/**
 * @brief ボイスのリリース処理を開始する。
 * @param voice 対象のボイス。
 */
void releaseVoice(Voice &voice);

/**
 * @brief ポルタメントを適用して位相インクリメントを更新する。
 * @param voice 対象のボイス。
 */
void updatePortamento(Voice &voice);

/**
 * @brief ボイスのエンベロープを更新する。
 * @param voice 対象のボイス。
 * @param attackStep アタック時の増分。
 * @param releaseStep リリース時の減分。
 */
void updateEnvelope(Voice &voice, int16_t attackStep, int16_t releaseStep);

}  // namespace mini_synth

