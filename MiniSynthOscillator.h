#pragma once

#include "MiniSynthTypes.h"

namespace mini_synth {

/**
 * @brief アナログ値から波形種別へ変換する。
 * @param value アナログ入力値。
 * @return 対応する波形種別。
 */
OscWaveform analogToWaveform(uint16_t value);

/**
 * @brief サイン波テーブルから値を取得する。
 * @param phase 正規化済み位相値。
 * @return サイン波サンプル値。
 */
int16_t sineFromTable(uint16_t phase);

/**
 * @brief ボイスの設定に基づいて波形を生成する。
 * @param voice 入力となるボイス情報。
 * @param waveform 選択されている波形種別。
 * @return 16bit の波形サンプル。
 */
int16_t renderWave(const Voice &voice, OscWaveform waveform);

}  // namespace mini_synth

