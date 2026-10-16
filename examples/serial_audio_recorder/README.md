# シリアル音声レコーダー (serial_audio_recorder)

Arduino Nano 33 BLE Sense Rev2 の内蔵 PDM マイクで取得した音声を USB シリアル経由で PC に連続転送し、PC 側で 1 秒ごとの WAV ファイルとして保存するサンプルです。

- 音声形式: 16 kHz / 16 bit / モノラル (符号付き PCM)
- 保存単位: 1 秒 (16,000 サンプル = 32,000 バイト) ごとに 1 ファイル

## 構成

| ファイル | 役割 |
| --- | --- |
| `serial_audio_recorder.ino` | マイコン側スケッチ。PDM マイクの音声をパケット化して USB シリアルへ送信 |
| `record_serial_audio.py` | PC 側受信ソフト。パケットを受信し、1 秒ごとに WAV を保存 |

## 1. マイコン側 (serial_audio_recorder.ino)

### 書き込み手順

1. Arduino IDE で `serial_audio_recorder.ino` を開きます。
2. ボードに **Arduino Nano 33 BLE** を選択します (Rev2 もこのボード定義を使用)。
3. 使用するシリアルポートを選択して書き込みます。

### 動作

- USB シリアル (115200 bps) への接続が確立されるまで待機し、接続後にマイクを開始します。
- 512 サンプルごとのブロックを小さなキュー (8 ブロック) に貯め、`loop()` でシリアルへ送信します。
- 各パケットの形式 (リトルエンディアン):

  | オフセット | サイズ | 内容 |
  | --- | --- | --- |
  | 0 | 4 バイト | 同期マーカー `AUD0` |
  | 4 | 2 バイト | シーケンス番号 (欠落検出用) |
  | 6 | 2 バイト | サンプル数 |
  | 8 | サンプル数 × 2 バイト | 16 bit PCM データ |

### 注意

- **Arduino IDE のシリアルモニターやシリアルプロッターは開かないでください。** ポートが占有され、バイナリデータが正しく受信できません。
- PC 側の受信ソフトを起動する前に、他のソフトがポートを使用していないことを確認してください。

## 2. PC 側 (record_serial_audio.py)

### 準備

Python 3 と pyserial をインストールします。

```sh
python -m pip install pyserial
```

### 実行方法

このディレクトリで、ポート名を指定して実行します。

```sh
python record_serial_audio.py --port COM5 --output-dir recordings
```

リポジトリのルートから実行する場合:

```sh
python examples/serial_audio_recorder/record_serial_audio.py --port COM5 --output-dir recordings
```

### オプション

| オプション | 必須 | 既定値 | 説明 |
| --- | --- | --- | --- |
| `--port` | はい | - | シリアルポート名 (例: Windows は `COM5`、macOS は `/dev/cu.usbmodem...`、Linux は `/dev/ttyACM0`) |
| `--baud` | いいえ | `115200` | ボーレート (スケッチ側と同じ値) |
| `--output-dir` | いいえ | `recordings` | WAV の保存先ディレクトリ (なければ作成) |

### 出力

- ファイル名は `audio_000001.wav`、`audio_000002.wav`、... と連番になります。
- 既存ファイルは上書きせず、空いている番号から続けて保存します。
- 1 秒ぶんのデータがそろうたびに `Saved ...` と表示されます。
- 停止するには `Ctrl+C` を押します。1 秒に満たない最後の端数は破棄されます。
- マイコン側のキューが溢れてブロックが欠落した場合は `Warning: N audio block(s) were dropped.` と表示されます。

## トラブルシューティング

- ポートが開けない: シリアルモニターなど他のソフトを閉じ、ポート名を確認してください (Arduino IDE の「ツール > ポート」)。
- 何も保存されない: スケッチが書き込まれているか確認し、受信ソフト起動後にボードをリセットしてください。
- 欠落の警告が頻発する: USB ケーブルや接続を変更するか、PC の負荷を下げてください。
