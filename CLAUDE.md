# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 必須ルール

- **日本語で返答する**
- **ビルドは実行しない** — コードの編集のみ行う
- **ファイルパスは推測せず、実ファイル構造を確認してから参照する**
- **'framework/~'の編集は原則行わない。フレームワーク側を変更すべきと判断した場合は都度指示を仰ぐ**

## ファイルエンコーディング

- `.h` / `.cpp` ファイル: **UTF-8 BOM付き**で作成（エンコーディング警告防止）
- `.hlsl` ファイル: **UTF-8 BOMなし**で作成（BOMがあるとシェーダーコンパイルエラーになる）

## ビルド

Visual Studio 2026 でソリューションファイル `GravityNotes.sln` を開いてビルドする。  
構成: Debug / Release、プラットフォーム: Win32 / x64。  
シェーダー（.hlsl）はビルド時に FX Compiler (DX11) で `.cso` バイトコードへ自動コンパイルされる。

## アーキテクチャ概要

DirectX 11 ベースの 3D グラフィクスアプリケーション（C++14）。  
全システムが **Init / Update / Draw / Finalize** の統一ライフサイクルで動作する。

### シーン管理

- `scene.cpp/h` — シーンのステートマシン。Title / Game / Result / DebugModel / DebugLighting / DebugScore の 6 シーンを管理
- `game.cpp/h` — メインゲームプレイシーン（3D + 2D の 2 パスレンダリング）
- `debugscene/` — モデル・ライティング・スコアの確認用デバッグシーン

### レンダリング

- **`shader/renderer.cpp/h`** — Direct3D 11 デバイス管理、レンダリングパイプラインの中核。`shader.cpp` や `framework/direct3d.cpp` は存在しないので参照しない
- `shader/shadermanager.h` — シェーダー種別（Unlit / Directional / Rim / Skinning / Grayscale）の列挙
- `shader/*.hlsl` → `shader/*.cso` — HLSL 5.0 シェーダー群。頂点スキニングは `shader_vertex_skinning.hlsl`

### 3D モデル・アニメーション

- `framework/model.cpp/h` — Assimp 経由の FBX / GLTF / OBJ 読み込み。ボーン行列最大 256 本のスケルタルアニメーション対応
- `framework/anim_sprite3d.cpp/h` — 3D スケルタルアニメーションコントローラ
- `framework/glb_model.cpp/h` — GLB フォーマット専用の補助ローダ

### カメラ・入力・オーディオ

- `framework/camera.cpp/h` — ピッチ制限（仰角 25°、俯角 -60°）付き 3D カメラ
- `framework/keyboard.cpp/h` / `mouse.cpp/h` — キーボード・マウス入力
- `framework/sound.cpp/h` — XAudio2 ベースの音声再生

### UI・ユーティリティ

- `framework/sprite2d.cpp/h` — 2D UI スプライト
- `framework/font.cpp/h` — stb_truetype によるフォントレンダリング
- `framework/fade.cpp/h` — シーン遷移フェード

## 解像度の扱い（`define.h`）

| 定数 | 値 | 用途 |
|---|---|---|
| `SCREEN_WIDTH/HEIGHT` | 1280×720 | UI要素の座標計算に使う |
| `DRAW_SCREEN_WIDTH/HEIGHT` | 3840×2160 | 実際の描画解像度（配置計算には使わない） |
| `DRAW_SCALE_X/Y` | 3.0 / 3.0 | 描画時のスケール倍率 |

スプライト等の座標は必ず `SCREEN_WIDTH/HEIGHT` 基準で指定し、`DRAW_SCREEN_*` を直接使わない。

## 主要な外部ライブラリ

- **Assimp** (`framework/assimp/`, `framework/library/assimp-vc143-mt.lib`) — 3D モデル読み込み
- **DirectXTex** (`framework/library/DirectXTex_Debug/Release.lib`) — テクスチャ処理
- **nlohmann/json** (ヘッダーオンリー) — JSON 設定ファイル読み込み
- **stb_truetype.h** (ヘッダーオンリー) — フォントラスタライズ
