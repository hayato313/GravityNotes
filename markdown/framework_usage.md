# フレームワーク使い方ガイド

---

## 目次
1. [初期化・終了の全体フロー](#初期化・終了の全体フロー)
2. [Renderer（描画エンジン）](#renderer描画エンジン)
3. [Camera（カメラ）](#cameraカメラ)
4. [Sprite2D（2Dスプライト）](#sprite2d2dスプライト)
5. [Sprite3D（3Dモデル）](#sprite3d3dモデル)
6. [AnimSprite3D（スケルタルアニメーション）](#animsprite3dスケルタルアニメーション)
7. [Sound（音声）](#sound音声)
8. [FontRenderer（テキスト描画）](#fontrendererテキスト描画)
9. [Fade（フェード遷移）](#fadeフェード遷移)
10. [Keyboard / Mouse（入力）](#keyboard--mouse入力)
11. [define.h 定数一覧](#defineh-定数一覧)

---

## 初期化・終了の全体フロー

各システムの初期化・終了は以下の順序で行う。

```cpp
// === 初期化 ===
InitRenderer(hInstance, hWnd, TRUE);
Sprite_Initialize();
Camera_Initialize();
Fade_Initialize();
Keyboard_Initialize();
Mouse_Initialize(hWnd);
Font_InitializeGlobalData();
InitSound();

// === 終了 ===
UninitSound();
Font_FinalizeGlobalData();
Camera_Finalize();
Fade_Finalize();
Sprite_Finalize();
FinalizeRenderer();
```

---

## Renderer（描画エンジン）

ヘッダ: `shader/renderer.h`

### 毎フレームの描画フロー

```cpp
Clear();             // バックバッファをクリア

// --- 3D描画 ---
SetDepthEnable(true);
// モデルのDraw()を呼ぶ

// --- 2D描画 ---
SetDepthEnable(false);
Sprite_BeginDraw2D();
// スプライトのDraw()を呼ぶ
Sprite_EndDraw2D();

Present();           // 画面に表示
```

### 行列・ライト設定

```cpp
SetWorldMatrix(worldMat);
SetViewMatrix(viewMat);
SetProjectionMatrix(projMat);
SetCameraPosition(XMFLOAT3(x, y, z));
SetLight(light);
```

### ブレンドステート

```cpp
SetBlendState(BLENDSTATE_NONE);   // 合成なし
SetBlendState(BLENDSTATE_ALFA);   // αブレンド（半透明）
SetBlendState(BLENDSTATE_ADD);    // 加算合成（光・炎エフェクト等）
SetBlendState(BLENDSTATE_SUB);    // 減算合成
```

### シェーダータイプ（SHADERTYPE）

| 値 | 説明 |
|---|---|
| `S_UNLIT` | ライティングなし。テクスチャ色そのまま |
| `S_LAMBERT` | ランバート拡散反射 |
| `S_PHONG` | フォン反射（デフォルト推奨） |
| `S_RIM_LIGHT` | リムライト |

---

## Camera（カメラ）

ヘッダ: `framework/camera.h`

グローバル関数で管理する。カメラはプレイヤーを中心とした球面座標系で動作し、マウスで操作される。

### 基本操作

```cpp
Camera_Initialize();          // 初期化（Init内で呼ぶ）
Camera_Finalize();            // 終了（Finalize内で呼ぶ）
Camera_Update();              // 毎フレーム呼ぶ（マウス入力処理込み）

// 注視対象の位置を毎フレーム渡す（プレイヤー追従）
Camera_SetTargetPos(playerPos);
```

### イベント・特殊操作

```cpp
// 指定した点へ0.25秒でカメラを向ける（イベント演出等）
Camera_LookAtPoint(XMFLOAT3(x, y, z));

// 入力スキップ（シーン開始直後のカメラぶれ防止）
GetCamera()->SkipNextInput(2);   // 2フレーム入力を無視

// カメラ設定変更
Camera_SetSensitivity(1.0f);     // マウス感度（デフォルト1.0）
Camera_SetDistance(6.0f);        // カメラ距離（define.hのCAMERA_DISTANCEを参照）
```

### カメラ情報の取得

```cpp
Camera* cam = GetCamera();
XMFLOAT3 pos   = cam->GetPos();       // カメラ位置
XMFLOAT3 atpos = cam->GetAtPos();     // 注視点
XMMATRIX view  = cam->GetView();      // ビュー行列
XMMATRIX proj  = cam->GetProjection(); // 投影行列
float yaw      = Camera_GetYaw();     // 水平角（ラジアン）
```

### ピッチ制限（define.hで設定）

- 上方向の限界: `PITCH_LIMIT_LOOK_UP = 25.0f`（度）
- 下方向の限界: `PITCH_LIMIT_LOOK_DOWN = -60.0f`（度）

---

## Sprite2D（2Dスプライト）

ヘッダ: `framework/sprite2d.h`

座標は `SCREEN_WIDTH(1280) × SCREEN_HEIGHT(720)` の論理座標系で指定する。

### 基本スプライト

```cpp
// コンストラクタ
Sprite2D sprite(
    XMFLOAT2(640.0f, 360.0f),          // 中心位置（論理座標）
    XMFLOAT2(100.0f, 100.0f),          // サイズ（幅, 高さ）
    0.0f,                               // 回転角度（度）
    XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), // 色（RGBA、0.0〜1.0）
    BLENDSTATE_ALFA,                    // ブレンドステート
    L"asset\\texture\\image.png"        // テクスチャパス
);

// 描画（Sprite_BeginDraw2D()〜Sprite_EndDraw2D()の間で呼ぶ）
sprite.Draw();

// 色・フリップ変更
sprite.SetColor(XMFLOAT4(1.0f, 0.5f, 0.5f, 0.8f));
sprite.SetFlipType(FLIPTYPE2D_HORIZONTAL);  // 左右反転
```

### 分割テクスチャ（アニメーション等）

```cpp
SplitSprite split(
    XMFLOAT2(100.0f, 100.0f),
    XMFLOAT2(64.0f, 64.0f),
    0.0f,
    XMFLOAT4(1, 1, 1, 1),
    BLENDSTATE_ALFA,
    L"asset\\texture\\spritesheet.png",
    4,   // 横4分割
    2    // 縦2分割（計8コマ）
);

split.SetTextureNumber(3);  // 0〜7 のコマを指定
split.Draw();
```

### クリック判定付きスプライト

```cpp
ClickSprite2D button(/* Sprite2Dと同じ引数 */);
button.Draw();

if (button.IsClick()) {
    // クリックされた
}
```

### FLIPTYPE2D

| 値 | 説明 |
|---|---|
| `FLIPTYPE2D_NONE` | 反転なし |
| `FLIPTYPE2D_HORIZONTAL` | 左右反転 |
| `FLIPTYPE2D_VERTICAL` | 上下反転 |
| `FLIPTYPE2D_BOTH` | 両方反転 |

---

## Sprite3D（3Dモデル）

ヘッダ: `framework/sprite3d.h`

アニメーションなしの3Dモデル表示に使う。アニメーションが必要な場合は `AnimSprite3D` を使う。

```cpp
Sprite3D model(
    XMFLOAT3(0.0f, 0.0f, 0.0f),    // 位置
    XMFLOAT3(1.0f, 1.0f, 1.0f),    // スケール
    XMFLOAT3(0.0f, 0.0f, 0.0f),    // 回転（度、XYZ順）
    "asset/model/object.fbx",        // モデルパス
    S_PHONG                          // シェーダータイプ
);

// 描画（SetDepthEnable(true)の状態で呼ぶ）
model.Draw();

// 色変更（マテリアル色を維持しつつ重ねる）
model.SetColor(1.0f, 0.5f, 0.5f, 1.0f);
model.SetColorAlpha(0.5f);           // 半透明
model.ResetColor();                  // マテリアル色に戻す

// サイズ取得
XMFLOAT3 originalSize = model.GetModelSize();    // 元のモデルサイズ
XMFLOAT3 displaySize  = model.GetDisplaySize();  // スケール後のサイズ
```

### Transform3D（基底クラス）の主要プロパティ

```cpp
model.pos   = XMFLOAT3(x, y, z);
model.scale = XMFLOAT3(sx, sy, sz);
model.rot   = XMFLOAT3(rx, ry, rz);
```

---

## AnimSprite3D（スケルタルアニメーション）

ヘッダ: `framework/anim_sprite3d.h`

`Sprite3D` のサブクラス。FBX/GLBに埋め込まれたアニメーションを再生できる。

### 基本フロー

```cpp
AnimSprite3D* chara = new AnimSprite3D(
    XMFLOAT3(0.0f, 0.0f, 0.0f),
    XMFLOAT3(1.0f, 1.0f, 1.0f),
    XMFLOAT3(0.0f, 0.0f, 0.0f),
    "asset/model/character.fbx",
    S_PHONG
);

// 再生（名前指定）
chara->PlayAnimationByName("Walk", true);   // ループあり
chara->PlayAnimationByName("Attack", false); // ループなし

// 再生（インデックス指定）
chara->PlayAnimationByIndex(0, true);

// 毎フレーム更新
float dt = 1.0f / FPS;
chara->UpdateAnimation(dt);
chara->UpdateBoneMatrices();

// 描画
chara->Draw();

// 終了
delete chara;
```

### アニメーション切り替え（ブレンド遷移）

```cpp
// ブレンド時間を設定してから切り替えると滑らかに遷移する
chara->SetAnimationBlendDuration(0.2);      // 0.2秒でブレンド
chara->PlayAnimationByName("Run");

bool blending = chara->IsAnimationBlending(); // ブレンド中かどうか
```

### アニメーション情報取得

```cpp
unsigned int count = chara->GetAnimationCount();
for (unsigned int i = 0; i < count; i++) {
    const char* name = chara->GetAnimationName(i);
    // 名前を確認してインデックスを把握する
}
```

### 制御

```cpp
chara->PauseAnimation();            // 一時停止
chara->ResumeAnimation();           // 再開
chara->StopAnimation();             // 停止
bool playing = chara->IsAnimationPlaying();
```

---

## Sound（音声）

ヘッダ: `framework/sound.h`

XAudio2ベース。MP3ファイルに対応。

```cpp
// 初期化・終了
InitSound();
UninitSound();

// 読み込み
SoundData* bgm = LoadMP3(L"asset/sound/bgm.mp3");
SoundData* se  = LoadMP3(L"asset/sound/jump.mp3");

// 再生
PlaySound(bgm, true);    // ループあり（BGM向け）
PlaySound(se,  false);   // ループなし（SE向け）

// 停止
StopSound(bgm);
StopSound(se);

// 解放
UnloadSound(bgm);
UnloadSound(se);

// マスターボリューム
SetMasterVolume(0.5f);   // 0.0〜1.0

// 再生位置取得（秒）
double sec = GetPlaybackPositionSec(bgm);
```

推奨ボリューム（`define.h` 参照）
- BGM: `SOUND_BGM_VOLUME = 0.2f`
- SE: `SOUND_SE_VOLUME = 0.7f`

---

## FontRenderer（テキスト描画）

ヘッダ: `framework/font.h`

座標はSprite2Dと同様、`SCREEN_WIDTH × SCREEN_HEIGHT` の論理座標で指定する。

```cpp
// 初期化・終了
Font_InitializeGlobalData();
Font_FinalizeGlobalData();

// テキスト表示
FontRenderer* label = new FontRenderer(
    XMFLOAT2(100.0f, 50.0f),            // 位置
    32.0f,                               // フォントサイズ（px）
    0.0f,                                // 回転（度）
    XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),  // 色
    "Score: 0"                           // テキスト（日本語対応）
);

// 描画（Sprite_BeginDraw2D()〜EndDraw2D()の間で呼ぶ）
label->Draw();

// テキスト・色の更新
label->SetText("Score: 100");
label->SetColor(XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f));  // 黄色

delete label;
```

フォントは `KaiseiDecol-Medium.ttf`（日本語対応）。グリフは2048×2048のアトラスにLRUキャッシュされる。

---

## Fade（フェード遷移）

ヘッダ: `framework/fade.h`

### 基本使用

```cpp
// 初期化・終了
Fade_Initialize();
Fade_Finalize();

// 毎フレーム（Update・Drawともに必ず呼ぶ）
Fade_Update();
Fade_Draw();     // 2D描画の最後に呼ぶ（他のUIより手前に描画される）
```

### シーン遷移

```cpp
// フェードアウト→シーン遷移→フェードイン（自動）
StartFade(SCENE_GAME);   // SCENE_GAMEへ遷移開始

// フェード状態確認
FADESTAT state = GetFadeState();
if (state == FADE_NONE) {
    // フェード完了
}
```

### シーン遷移なしのフェード（マップ暗転等）

```cpp
StartFade(SCENE_NONE);   // フェードアウト（真っ暗で停止）
// 暗転中に処理...
Fade_StartIn();          // フェードイン開始
```

### FADESTAT

| 値 | 状態 |
|---|---|
| `FADE_NONE` | フェード中でない |
| `FADE_OUT` | 暗くなっている最中 |
| `FADE_IN` | 明るくなっている最中 |
| `FADE_MAX` | 完全暗転で待機中 |

フェードの速度: α ±0.05f/フレーム（約20フレーム＝1/3秒）

---

## Keyboard / Mouse（入力）

ヘッダ: `framework/keyboard.h`, `mouse.h`

### キーボード

```cpp
Keyboard_Initialize();

// 押し続け判定
if (Keyboard_IsKeyDown(KK_W)) { /* 移動 */ }

// 押した瞬間のみ（トリガー）
if (Keyboard_IsKeyDownTrigger(KK_SPACE)) { /* ジャンプ */ }

// 離した瞬間のみ
if (Keyboard_IsKeyUp(KK_ESCAPE)) { /* メニュー */ }
```

主要キー: `KK_A〜KK_Z`, `KK_LEFT/RIGHT/UP/DOWN`, `KK_SPACE`, `KK_ENTER`, `KK_ESCAPE`, `KK_LEFTSHIFT`, `KK_LEFTCONTROL`

### マウス

```cpp
Mouse_Initialize(hWnd);
Mouse_Finalize();

Mouse_State ms;
Mouse_GetState(&ms);

// ボタン判定
ms.leftButton   // 左クリック
ms.rightButton  // 右クリック
ms.middleButton // 中クリック

// 座標（絶対モード時: スクリーン座標、相対モード時: 移動量）
ms.x, ms.y

// スクロール
ms.scrollWheelValue
Mouse_ResetScrollWheelValue();

// 3Dカメラ操作用（Cameraクラスが内部で使用）
Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);  // 相対座標モード
Mouse_SetMode(MOUSE_POSITION_MODE_ABSOLUTE);  // 絶対座標モード
Mouse_SetVisible(false);  // カーソル非表示
LockMouse();              // マウスカーソルをウィンドウ中央に固定
UnLockMouse();
```

---

## define.h 定数一覧

```cpp
// 解像度
SCREEN_WIDTH     = 1280.0f  // UI座標計算に使う（この値を基準に配置する）
SCREEN_HEIGHT    = 720.0f
DRAW_SCREEN_WIDTH  = 3840.0f  // 実際の描画解像度（配置計算には使わない）
DRAW_SCREEN_HEIGHT = 2160.0f
DRAW_SCALE_X     = 3.0f     // 描画倍率（DRAW/SCREEN）
DRAW_SCALE_Y     = 3.0f

// フレームレート
FPS = 60

// カメラ
PITCH_LIMIT_LOOK_UP   =  25.0f  // 上方向の限界（度）
PITCH_LIMIT_LOOK_DOWN = -60.0f  // 下方向の限界（度）
MOUSE_SENSITIVITY     =   0.15f
CAMERA_DISTANCE       =   6.0f
CAMERA_OFFSET_Y       =   1.5f  // 注視点のY方向オフセット

// 音声
SOUND_BGM_VOLUME = 0.2f
SOUND_SE_VOLUME  = 0.7f
```

---

## よくある注意点

- **座標指定は必ず `SCREEN_WIDTH/HEIGHT` 基準**で行う。`DRAW_SCREEN_*` を位置・サイズ計算に直接使わない。
- **2D描画は必ず `Sprite_BeginDraw2D()` と `Sprite_EndDraw2D()` で挟む**。FontRendererのDrawもこの間で呼ぶ。
- **3D描画中は `SetDepthEnable(true)`**、2D描画前に `SetDepthEnable(false)` に切り替える。
- **`Fade_Draw()` は2D描画ブロックの末尾**（他のUIより前面）で呼ぶ。
- **`.h/.cpp` はUTF-8 BOM付き**、**`.hlsl` はUTF-8 BOMなし**で保存する。
