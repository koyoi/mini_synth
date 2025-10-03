## 概要
- Arduino を用いて STM32 マイコンでアナログシンセ風の機能を実現するプロジェクト。

## 使用環境など
- 開発環境: Arduino 2 系
- 現在の推奨ボード: NUCLEO‑F411RE（F411 を利用）
- サウンドエンジン: Mozzi2（デフォルトは PWM/DAC 出力）

## 入力
- アナログポット (ADC)
  - `kOscSelectPin` (A0): 波形選択
  - `kAttackPin` (A1): アタック（ASR/EG）
  - `kReleasePin` (A2): リリース（ASR/EG）
  - `kFilterPin` (A3): フィルタ カットオフ
  - `kResonancePin` (A4): フィルタ レゾナンス
- デジタル鍵盤（直接 GPIO、5 鍵）
  - ピン: `kKeyPin0`..`kKeyPin4`（現在は 2,3,4,5,6）
  - 割当ノート: `kKeyNotes` = {C4, E4, G4, A4, D4}（60,64,67,69,62）
  - 挙動: 押している間オン（押下で NoteOn、離すと NoteOff）
  - 将来的にはダイオードありのマトリクススキャンに置換予定（後述）
- シリアル MIDI 入力
  - `Serial1` を使用（MIDI IN はオプトカプラ推奨）

## 出力
- I2C LCD（表示用）
  - OSC 波形やパラメータ表示に利用予定（I2C 接続）
- オーディオ出力
  - デフォルト: Mozzi の PWM/DAC 出力
  - 将来的: I2S + 外部 DAC（例: PCM5102A）へ移行予定（I2S 実装は後回し、README にメモあり）

## 機能（実装状況: 2025-10-04）
- OSC（実装済）: Sin/Triangle/Saw/Pulse/Square
- ポリフォニック: 4 音（後着優先、実装済）
- ポルタメント: 実装済（押している間ピッチが移る）
- エンベロープ: ASR 相当は実装済（ADSR の Decay/Sustain レベルは未実装）
- フィルタ: SVF 実装済、ビルドスイッチで切替可能
  - デフォルト: `GLOBAL_SVF`（ミックス後に SVF）
  - オプション: `VOICE_SVF`（`-DVOICE_SVF=1`、ボイス毎に SVF、キー追従）
- レゾナンス: グローバルノブで制御（将来的に per-voice Q を追加可）
- ノート→f テーブル: 実装済（`MiniSynthNoteTable.h`、`tools/generate_note_table.py` で再生成可能）
- パラメータスムージング/保護: control→audio の 1-pole スムージングとソフトクリップ実装済

## ハードウェアメモ / 今後の予定
- I2S: 将来的に I2S+外付け DAC（PCM5102A 等）を検討。現時点では I2S 実装は後回し。`MiniSynthI2S.*` にテンプレートを用意済。
- 鍵盤: 将来的にダイオード付きマトリクススキャンへ移行予定（5x6 など）。現行は 5 鍵の直接 GPIO 実装で動作確認用。

## 開発メモ
- ビルドスイッチ
  - `-DVOICE_SVF=1` : ボイス毎 SVF を有効化（CPU/メモリ負荷増）
  - `-DUSE_I2S=1` : I2S 出力を有効化（NUCLEO‑F411RE 向け HAL テンプレートあり。CubeMX の設定が必要）

### CPU 負荷 (Mozzi) の取得

- 実装: `MiniSynthCpuLoad.*` により、Mozzi のオーディオコールバック実行時間を計測し、コントロール周期で使用率 (%) を算出します。
- 有効化方法:
  - `MiniSynthCpuLoad.h` の `#define CPU_LOAD_DEBUG 1` を有効にすると、`handleControl()` の中で計測値を Serial に出力します（`Serial.begin()` が必要）。
  - `cpuLoadEnter()` / `cpuLoadExit()` はオーディオ生成コールの前後に自動で挿入済みです。
- 出力: 毎コントロール周期に計測された滑らかな CPU 使用率（0..100%）が算出されます。
- 注意:
  - 測定は micros() を利用しており、非常に短いコールでは分解能の制約がありますが、Mozzi のオーディオレート (例: 16384Hz) での平均値取得には十分です。
  - Arduino IDE / ボード固有の最適化や割り込みの影響で値が変動します。実際の負荷は I2S や HAL の割り込み処理も含めたシステム全体の挙動で評価してください。

---

## 実装状況（2025-10-04）

以下は現在のソースツリーに実装されている機能と未実装の点を簡潔にまとめたものです。

- ハードウェアターゲット
  - 当初は STM32F103 を想定していましたが、フラッシュ容量の制約により STM32F411 系（例: Nucleo‑F411）での運用を推奨します。F103 では一部の追加機能がメモリ不足になります。

- オシレータ（実装済み）
  - Sin、Triangle、Saw、Pulse、Square を実装。
  - 4音ポリ（後着優先）。

- エンベロープ（部分実装）
  - 現在は ASR（Attack / Sustain / Release）相当が実装されています。`kAttackPin`/`kReleasePin` で Attack/Release の速度を制御します。
  - 典型的な ADSR（Decay や Sustain レベルの独立した調整）は未実装です。

- ポルタメント（実装済み）
  - `updatePortamento()` により滑らかなピッチ移行（ポルタメント）を行います。

- フィルタ（実装済み、切替可能）
  - State Variable Filter (SVF) を実装しました。ビルド時に以下の方式を選択できます：
    - `GLOBAL_SVF`（デフォルト）: ミックス後に一台の SVF を適用（パート単位の色付けに適する）
    - `VOICE_SVF`（ビルド時に `-DVOICE_SVF=1` を指定）: 各ボイスごとに SVF を持ち、キー追従でカットオフを変化させる（より表現力が高いがメモリ／CPU 負荷が増える）
  - レゾナンスは現在グローバルノブ（`kResonancePin`）で制御されます。将来的にボイス毎 Q を追加可能です。

- レゾナンス安定化とスムージング（実装済み）
  - controlRate→audioRate のパラメータスムージング（1-pole）と、簡易ソフトクリップを導入して発振やステップノイズを抑制しています。

- ノート→フィルタ係数テーブル（実装済み）
  - MIDI ノート 0..127 に対する SVF 正規化周波数係数 `kNoteFTable` を生成し、`MiniSynthNoteTable.h` に埋め込んでいます（テーブルは tools/generate_note_table.py で再生成可能）。

- Mozzi / オーディオ出力
  - 現状は Mozzi の PWM/DAC 出力を想定しており、まずは PWM を使った出力で動作させる設計です。将来的には I2S + 外部 DAC（例: PCM5102A）への移行を想定しています。

- 未実装／今後の課題
  - ADSR の Decay / Sustain レベル制御（未実装）
  - per-voice Q（必要に応じて追加予定）
  - Mozzi から I2S へ直接出力するラッパ（将来的な移行）

以上を README に反映しました。その他、実装の詳細やビルド方法（`-DVOICE_SVF=1` など）を README に追記したい場合は指定してください。

