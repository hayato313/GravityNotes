# 譜面 JSON フォーマット仕様書

ゲームプロジェクト側でリズムアクションゲームの譜面を読み込む際の JSON フォーマット定義書です。

---

## ファイル構造

```json
{
  "musicname":   "Test Stage",
  "musicauthor": "Artist Name",
  "scoreauthor": "Charter Name",
  "difficulty":  10.0,
  "bpm":         160.0,
  "thumbnail":   "test.png",
  "music":       "test.mp3",
  "events": [ ... ]
}
```

### トップレベルフィールド

| フィールド    | 型      | 必須 | デフォルト | 説明 |
|--------------|---------|------|-----------|------|
| `musicname`   | string  | No  | `""`      | 楽曲タイトル |
| `musicauthor` | string  | No  | `""`      | 楽曲制作者名 |
| `scoreauthor` | string  | No  | `""`      | 譜面制作者名 |
| `difficulty`  | float   | No  | `0.0`     | 難易度（任意のスケール、表示用） |
| `bpm`         | float   | No  | `120.0`   | 曲の BPM（beats per minute） |
| `thumbnail`   | string  | No  | `""`      | サムネイル画像ファイル名（ファイル名のみ） |
| `music`       | string  | No  | `""`      | 音楽ファイル名（ファイル名のみ、ディレクトリパスは含まない） |
| `events`      | array   | Yes | `[]`      | イベントオブジェクトの配列 |

#### `music` / `thumbnail` パスの解決ルール

これらのフィールドにはファイル名のみを記載します。ゲーム側は以下のパスで読み込みます:

```
Assets/music/<music の値>
Assets/thumbnail/<thumbnail の値>
```

例: `"music": "test.mp3"` → `Assets/music/test.mp3`

---

## イベントオブジェクト

`events` 配列の各要素は以下のフィールドで構成されます。

### 共通フィールド（全ノーツ種類共通）

| フィールド | 型      | 必須 | 説明 |
|-----------|---------|------|------|
| `beat`    | float   | Yes  | イベントが発生する beat 番号（0.0 以上） |
| `type`    | string  | Yes  | ノーツ種類（後述の定義値参照） |
| `wall`    | string  | Yes  | 配置する壁（後述の定義値参照） |
| `lane`    | integer | Yes  | 壁内のレーン番号（`0` / `1` / `2`） |

### Hold 専用フィールド

`type` が `"Hold"` の場合のみ追加で必要なフィールドです。

| フィールド | 型     | 必須 | デフォルト    | 説明 |
|-----------|--------|------|--------------|------|
| `endBeat` | float  | Yes  | `beat` と同値 | Hold の終了 beat |
| `endWall` | string | Yes  | `wall` と同値 | Hold の終了壁（壁またぎ対応） |

---

## 定義値

### `type`（ノーツ種類）

| 値          | 説明 |
|-------------|------|
| `"Tap"`     | 単発ノーツ（通常敵） |
| `"Hold"`    | 長押しノーツ（壁またぎ可） |
| `"Orb"`     | オーブノーツ |
| `"Barrier"` | バリアノーツ |

> **拡張ガイドライン**  
> 新しいノーツ種類を追加する場合は、`type` に新しい文字列値を定義します。  
> 既存の値は変更せず、既知の `type` 値以外は**スキップ（無視）** する実装にしてください。  
> これにより古いバージョンのゲームでも譜面ファイルの互換性が保たれます。

### `wall`（壁）

| 値        | 説明 |
|-----------|------|
| `"Up"`    | 上壁 |
| `"Left"`  | 左壁 |
| `"Down"`  | 下壁 |
| `"Right"` | 右壁 |

### `lane`（レーン）

壁ごとに 0〜2 の 3 レーンが存在します。

| 値 | 位置 |
|----|------|
| `0` | 左（または上）端 |
| `1` | 中央 |
| `2` | 右（または下）端 |

> **Hold のレーン**  
> Hold ノーツは常に `lane: 1`（中央）に配置されます。

---

## beat ベースのタイミング

イベントの位置は **beat 単位** で管理されます。秒への変換は以下の式で行います:

```
time(seconds) = (60.0 / bpm) * beat
```

BPM が変化した場合、beat 基準のタイムスタンプは引き続き有効ですが、秒ベースの値は無効になります。  
ゲーム側は必ず beat → 秒変換を実行時に行い、秒ベースで直接タイミングを保持しないでください。

---

## ステージ構成

```
壁 4 面 × レーン 3 本 = 合計 12 レーン
```

物体はカメラ奥から手前に向かってスクロールしてきます。

---

## 完全なサンプル

```json
{
  "musicname":   "Test Stage",
  "musicauthor": "Artist Name",
  "scoreauthor": "Charter Name",
  "difficulty":  10.0,
  "bpm":         160.0,
  "thumbnail":   "test.png",
  "music":       "test.mp3",
  "events": [
    {
      "beat": 4.0,
      "type": "Tap",
      "wall": "Up",
      "lane": 1
    },
    {
      "beat": 8.0,
      "type": "Hold",
      "wall": "Left",
      "lane": 1,
      "endBeat": 10.0,
      "endWall": "Down"
    },
    {
      "beat": 12.0,
      "type": "Orb",
      "wall": "Down",
      "lane": 0
    },
    {
      "beat": 16.0,
      "type": "Barrier",
      "wall": "Right",
      "lane": 2
    }
  ]
}
```

---

## 読み込み時のデフォルト値

パースエラー回避のため、フィールドが欠落している場合は以下の値にフォールバックします。

| フィールド    | フォールバック値 |
|--------------|----------------|
| `musicname`   | `""` |
| `musicauthor` | `""` |
| `scoreauthor` | `""` |
| `difficulty`  | `0.0` |
| `bpm`         | `120.0` |
| `thumbnail`   | `""` |
| `music`       | `""` |
| `events`      | `[]` |
| `beat`        | `0.0` |
| `type`        | `"Tap"` |
| `wall`        | `"Up"` |
| `lane`        | `0` |
| `endBeat`     | `beat` と同値 |
| `endWall`     | `wall` と同値 |

---

## 実装上の注意事項

- **未知の `type` 値は無視する**: 将来の拡張に備え、パーサーは未知のノーツ種類をエラーにせずスキップしてください。
- **`events` は beat 昇順でソートされている**: ゲームエンジン側は順序に依存してよいですが、ソートされていない場合にもクラッシュしない実装を推奨します。
- **Hold の `endBeat` は `beat` 以上**: `endBeat < beat` になるデータは不正です。読み込み時に `endBeat = max(beat, endBeat)` でガードしてください。
- **`lane` は 0〜2 の範囲**: 範囲外の値はクランプまたは無視してください。
