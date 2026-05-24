# 描画更新ループ解析 ＆ 音ゲー向け改善案

## 1. 現状のループ構造

### 1-1. タイマー初期化

```cpp
// main.cpp
timeBeginPeriod(1);  // Win32マルチメディアタイマーを1ms精度に設定
dwExecLastTime = dwFPSLastTime = dwTitleUpdateTime = timeGetTime();
```

`timeGetTime()` はシステム起動からのミリ秒を返す `DWORD` 値。  
`timeBeginPeriod(1)` で分解能を1msに引き上げているが、**1ms以下の精度は保証されない**。

---

### 1-2. メッセージループ全体像

```
WinMain ループ
│
├─ PeekMessage → メッセージがあれば WndProc に渡す
│
└─ ゲーム処理（メッセージなしの場合）
    │
    ├─ FPS カウンタ更新（1秒ごと）
    │
    └─ フレーム判定: (dwCurrentTime - dwExecLastTime) >= 1000/g_TargetFPS
        │
        ├─ dwExecLastTime = dwCurrentTime  ← ★問題点①
        │
        ├─ Alt+Enter / F11 ウィンドウ操作
        │
        ├─ Fade_Update() + Update()   → g_UpdateTime 計測
        │
        ├─ Clear()
        ├─ SetWorldViewProjection2D()
        ├─ Draw()
        ├─ Present(0, 0)              → g_DrawTime 計測
        │
        ├─ if (totalTime > 16666) { } ← ★問題点②（空実装）
        │
        └─ keycopy() / dwFrameCount++
```

---

### 1-3. フレーム判定の詳細

```cpp
// define.h
#define FPS (60)

// main.cpp
if ((dwCurrentTime - dwExecLastTime) >= ((float)1000 / g_TargetFPS))
```

| 項目 | 値 |
|------|----|
| 目標周期 | `1000 / 60 ≈ 16.667 ms` |
| タイマー精度 | `timeGetTime()` ≒ 1ms |
| Vsync | `Present(0, 0)` → **なし** |
| 遅延時のキャッチアップ | **なし**（後述） |

---

## 2. 現状の問題点

### 問題① `dwExecLastTime = dwCurrentTime`（時刻リセット）

フレームが遅延した場合、`dwExecLastTime` を「本来の開始予定時刻」ではなく  
「実際に処理を開始した時刻」にリセットしている。

```
理想:  |----16ms----|----16ms----|----16ms----|
現実:  |----16ms----|------20ms------|----16ms----|
           ↑通常          ↑フレームドロップ
現状の次フレーム開始: 遅延分だけずれた位置から再スタート → 累積誤差
```

音ゲーでは判定タイミングが絶対時刻に依存するため、  
この方式だと**フレーム落ちのたびに論理時刻がずれていく**。

### 問題② 遅延検知後の処理が空

```cpp
if (totalTime > 16666)
{
    // 何もしない
}
```

フレームドロップを検出しているが、対処していない。

### 問題③ `timeGetTime()` の精度限界

`timeGetTime()` は最大 1ms 精度。60FPS では 1 フレーム ≒ 16.667ms のため  
±1ms のジッターが相対誤差 **6%** に達する。  
音ゲーの判定ウィンドウ（PERFECT ±43ms 程度）には影響が出うる。

### 問題④ Update と Draw が常にセット

現状では 1 フレームに必ず 1回 Update + 1回 Draw が実行される。  
フレームドロップ時に「描画を飛ばして論理更新だけを複数回走らせる」仕組みがない。

---

## 3. 音ゲー向け：固定タイムステップ＋アキュムレータ方式

### 3-1. 考え方

```
論理更新(Update) は固定間隔で必ず実行する
描画(Draw)       は時間が余ったときだけ実行する（スキップ可）
```

フレームが落ちても論理時刻は壁時計と同期し続ける。

### 3-2. アキュムレータパターン（疑似コード）

```cpp
// 定数
const double FIXED_STEP = 1.0 / 60.0;   // 論理1ステップの秒数
const int    MAX_STEPS  = 5;             // 1フレームで実行する最大論理ステップ数
                                          // （スパイラル・オブ・デス防止）

double accumulator = 0.0;
double prevTime    = GetHighResTime();   // QueryPerformanceCounter 推奨

// --- メインループ内 ---
double now   = GetHighResTime();
double delta = now - prevTime;
prevTime     = now;

// 極端に大きいdeltaをクランプ（デバッガ停止などへの対処）
if (delta > 0.25) delta = 0.25;

accumulator += delta;

// 論理更新：固定ステップを消化する
int steps = 0;
while (accumulator >= FIXED_STEP && steps < MAX_STEPS)
{
    Update();        // 常に固定 FIXED_STEP 分だけ進める
    accumulator -= FIXED_STEP;
    steps++;
}

// 描画：アキュムレータの余剰を補間率として渡す（将来の拡張用）
double alpha = accumulator / FIXED_STEP;  // 0.0 〜 1.0
Clear();
Draw(alpha);   // 現在はalphaを無視してもよい
Present();
```

### 3-3. 高精度タイマーへの切り替え

現状の `timeGetTime()` → `QueryPerformanceCounter` または `std::chrono::high_resolution_clock` に置き換える。

```cpp
// 推奨: chrono（C++11以降、移植性が高い）
#include <chrono>
using Clock = std::chrono::steady_clock;  // 単調増加クロック（壁時計ズレなし）
using Seconds = std::chrono::duration<double>;

auto prevTime = Clock::now();

// ループ内
auto now   = Clock::now();
double delta = std::chrono::duration_cast<Seconds>(now - prevTime).count();
prevTime = now;
```

`steady_clock` は既に `main.cpp` で `g_UpdateTime` / `g_DrawTime` の計測に使われている。  
メインループのタイミング制御も同じクロックに統一するのが望ましい。

---

## 4. sound.cpp の現状と音ゲー向け拡張

### 4-1. 現在の実装概要

| 項目 | 実装内容 |
|------|---------|
| ライブラリ | XAudio2 + Media Foundation |
| デコード方式 | `MFCreateSourceReaderFromURL` で **全データを一括PCM展開** → `BYTE*` バッファへコピー |
| 再生 | `IXAudio2SourceVoice::SubmitSourceBuffer` → `Start()` |
| ループ | `XAUDIO2_LOOP_INFINITE` フラグ |
| 再生位置取得 | **実装なし** ← 音ゲーに必要な機能が未実装 |

`SoundData` 構造体には `pWfx`（`WAVEFORMATEX*`）が保持されているため  
`pWfx->nSamplesPerSec`（サンプルレート）は取得可能。

### 4-2. XAudio2 での再生位置取得

`IXAudio2SourceVoice::GetState()` が返す `SamplesPlayed` を使う。  
これは CPU タイマーと独立したハードウェア駆動のカウンタであり、音ゲーの判定時刻として**最も精度が高い**。

```cpp
// sound.h に追加するイメージ
double GetPlaybackPositionSec(const SoundData* data)
{
    if (!data || !data->pSourceVoice || !data->pWfx) return 0.0;
    XAUDIO2_VOICE_STATE state = {};
    data->pSourceVoice->GetState(&state);
    return static_cast<double>(state.SamplesPlayed) / data->pWfx->nSamplesPerSec;
}
```

`SamplesPlayed` は `UINT64` で、再生開始からの累計サンプル数。  
ループ再生時は繰り返しカウントアップし続けるため、ループ位置を別途管理する必要がある。

### 4-3. 現状の問題点

1. **再生位置取得 API がない** — `SoundData` に `GetPlaybackPositionSec()` が未実装  
2. **全データ一括デコード** — 長尺BGM（3〜5分）はメモリを大量消費する  
   → 音ゲーでは通常ストリーミング再生が望ましいが、現状は全展開方式  
3. **再生開始時刻の記録がない** — `steady_clock` との同期オフセットを計算できない

### 4-4. 推奨: 判定時刻の取得フロー（実装案）

```
音楽スタート時:
  g_MusicStartSample = 0  (リセット)
  PlaySound(bgmData, false)

毎フレーム判定処理内:
  double musicTimeSec = GetPlaybackPositionSec(bgmData)
  // ↑ CPUタイマーに頼らない、XAudio2ハードウェアカウンタ基準

入力イベント発生時:
  double hitTimeSec = GetPlaybackPositionSec(bgmData)
  float  diff       = hitTimeSec - note->timeSec   // 正=遅い, 負=早い
  Judge(diff)
```

---

## 5. 音ゲー特有の考慮点

### 5-1. 音楽再生位置を「正」の時刻軸にする

```
音楽再生位置 [sec] を基準にノーツの判定を行う
フレームカウンタ × FIXED_STEP は補助情報として使う
```

XAudio2 の `SamplesPlayed` は CPU タイマーと独立した  
高精度タイムソースであり、音ゲーの判定基準として最適。

### 5-2. 入力サンプリング

`keycopy()` を Update の外（ループ先頭）で毎ループ実行し、  
「押した瞬間のフレーム時刻」を記録する。

```cpp
// 毎ループ先頭
Keyboard_Update();
auto inputTimestamp = Clock::now();  // 入力を受け取った壁時計時刻

// 論理ステップ内
if (IsKeyTriggered(KEY_SPACE))
{
    double hitTime_sec = /* inputTimestamp を論理時刻に変換 */;
    Judge(hitTime_sec);
}
```

### 5-3. `Present` の Vsync フラグ

現状: `Present(0, 0)` → Vsync なし（ティアリング発生の可能性あり）  
音ゲーでは視覚的なノーツ位置精度が重要なため、Vsync 1 が望ましいケースもある。  
ただし Vsync 1 にすると `Present` でブロックされ論理更新が遅延するリスクがある。  
→ **論理更新スレッドと描画スレッドを分離**することで両立可能（将来の拡張）。

---

## 6. 現状コードへの最小変更案（段階的対応）

### Step 1: タイマーを `steady_clock` に統一

`dwExecLastTime` / `timeGetTime()` を `std::chrono::steady_clock` に置き換え。

### Step 2: アキュムレータ導入

```cpp
// main.cpp グローバル追加
static double g_Accumulator = 0.0;
static auto   g_PrevFrameTime = std::chrono::steady_clock::now();
static constexpr double FIXED_STEP = 1.0 / FPS;
```

```cpp
// ループ内の置き換え（PeekMessage の else ブロック）
auto now = std::chrono::steady_clock::now();
double delta = std::chrono::duration<double>(now - g_PrevFrameTime).count();
g_PrevFrameTime = now;
if (delta > 0.25) delta = 0.25;

g_Accumulator += delta;

int steps = 0;
while (g_Accumulator >= FIXED_STEP && steps < 5)
{
    Keyboard_Update();  // 入力を毎ステップ更新
    Fade_Update();
    Update();
    g_Accumulator -= FIXED_STEP;
    steps++;
}

// 描画は毎ループ1回
Clear();
SetWorldViewProjection2D();
Draw();
Present(0, 0);
```

### Step 3: `GetPlaybackPositionSec()` を sound.h/cpp に追加

```cpp
// sound.h に追加
double GetPlaybackPositionSec(const SoundData* data);

// sound.cpp に追加
double GetPlaybackPositionSec(const SoundData* data) {
    if (!data || !data->pSourceVoice || !data->pWfx) return 0.0;
    XAUDIO2_VOICE_STATE state = {};
    data->pSourceVoice->GetState(&state);
    return static_cast<double>(state.SamplesPlayed) / data->pWfx->nSamplesPerSec;
}
```

### Step 4: 音楽時刻との同期（音ゲー本実装時）

`Update()` 内で `GetPlaybackPositionSec(bgmData)` を呼び、  
判定処理でその値を使用する。

---

## 7. まとめ

| 観点 | 現状 | 音ゲー向け推奨 |
|------|------|---------------|
| タイマー精度 | `timeGetTime()` ≒ 1ms | `steady_clock` ≒ sub-μs |
| フレームドロップ時の論理更新 | スキップ・遅延する | アキュムレータで追いつく |
| Update/Draw の分離 | セット | 論理更新を複数回実行可能 |
| 判定の時刻基準 | フレームカウンタ | **XAudio2 `SamplesPlayed`** |
| 再生位置取得 API | **未実装** | `GetPlaybackPositionSec()` 追加 |
| Vsync | なし | 要検討（論理とレンダーを分離して両立） |

---

*ファイル: `framework/main.cpp`, `shader/renderer.cpp`, `define.h`*  
*作成日: 2026-05-22*
