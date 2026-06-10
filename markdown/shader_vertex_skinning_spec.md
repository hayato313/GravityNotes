# Vertex Skinning シェーダー移植要件仕様書

## 概要

`shader_vertex_skinning.hlsl` と `AnimSprite3D` クラスを他プロジェクトへ移植するための実装要件をまとめる。  
スキニング計算はすべて **頂点シェーダー側** で行われる GPU スキニング方式（Linear Blend Skinning）。

---

## 1. シェーダーファイル構成

| ファイル | 役割 |
|---|---|
| `Common.hlsl` | ライトやマクロの共通定義（`#include` 対象） |
| `shader_vertex_skinning.hlsl` | スキニング頂点シェーダー本体 |
| ピクセルシェーダーは別ファイル（本仕様の対象外） |

---

## 2. Common.hlsl の定義要件

既存のLight実装に倣う

```

- `shader_vertex_skinning.hlsl` の `cbuffer Buffer2` で `POINT_LIGHT` 型を使用するため必須。
- ライティングを使用しない場合でも、**定義だけは存在しなければコンパイルエラーになる**。

---

## 3. 定数バッファ（cbuffer）一覧

| レジスタ | 名称 | 内容 | バイト数 |
|---|---|---|---|
| `b0` | `Buffer0` | WVP 合成行列（View × Projection 込み） | 64 byte |
| `b1` | `Buffer1` | ワールド変換行列 | 64 byte |
| `b2` | `Buffer2` | ポイントライト配列 × 16 + カウント + パディング + アンビエント | 可変（約 1056 byte） |
| `b5` | `BoneBuffer` | ボーン行列配列 × 256 | 256 × 64 = 16384 byte |

### 3-1. Buffer0（b0）

```hlsl
cbuffer Buffer0 : register(b0)
{
	float4x4 mtx;   // View × Projection × World（クリップ空間変換用）
};
```

- 頂点の最終クリップ座標 `SV_POSITION` の計算に使用。
- `mul(skinnedPos, mtx)` の形で適用される。

### 3-2. Buffer1（b1）

```hlsl
cbuffer Buffer1 : register(b1)
{
	float4x4 worldMtx;  // ワールド変換行列
};
```

- スキニング後の頂点座標をワールド空間に変換（ライティング計算用の `worldPos` 出力）。
- スキニング後の法線ベクトルをワールド空間に変換（法線出力用）。

### 3-3. Buffer2（b2）

```hlsl
cbuffer Buffer2 : register(b2)
{
	POINT_LIGHT PointLights[MAX_POINT_LIGHTS]; // MAX_POINT_LIGHTS = 16
	int         PointLightCount;
	float3      LightPadding;
	float4      AmbientColor;
};
```

- このシェーダー（頂点シェーダー）では **ライトデータを直接使わない**。  
  ピクセルシェーダーと共通レジスタを使うため、定義として含める必要がある。
- ライティング不要のプロジェクトでは削除可だが、ピクセルシェーダーとの整合に注意。

### 3-4. BoneBuffer（b5）

```hlsl
#define MAX_BONES 256

cbuffer BoneBuffer : register(b5)
{
	float4x4 BoneMatrices[MAX_BONES];
};
```

- **最大 256 本**のボーン行列を格納。
- C++ 側の `BoneMatrices::MAX_BONES`（= 256）と必ず一致させること。
- レジスタ番号 `b5` を使う理由は `b0`～`b4` を他用途（カメラ・マテリアル等）に予約しているため。  
  移植先でレジスタ番号を変える場合は C++ 側の `VSSetConstantBuffers` 呼び出しも同時に変更する。

---

## 4. 頂点入力レイアウト（VS_INPUT）

```hlsl
struct VS_INPUT
{
	float3 posL       : POSITION0;      // ローカル空間座標
	float3 normal     : NORMAL0;        // ローカル法線
	float4 color      : COLOR0;         // 頂点カラー
	float2 texcoord   : TEXCOORD0;      // UV座標
	uint4  boneIndex  : BLENDINDICES0;  // 影響ボーンのインデックス（最大4本）
	float4 boneWeight : BLENDWEIGHT0;   // 各ボーンの影響ウェイト（合計 ≤ 1.0）
};
```

### C++ 側の対応頂点構造体（要件）

```cpp
struct SkinnedVertex
{
	DirectX::XMFLOAT3 position;   // POSITION0
	DirectX::XMFLOAT3 normal;     // NORMAL0
	DirectX::XMFLOAT4 color;      // COLOR0
	DirectX::XMFLOAT2 texcoord;   // TEXCOORD0
	UINT   boneIndex[4];          // BLENDINDICES0  (DXGI_FORMAT_R32G32B32A32_UINT)
	float  boneWeight[4];         // BLENDWEIGHT0   (DXGI_FORMAT_R32G32B32A32_FLOAT)
};
```

### Input Layout 定義例

```cpp
D3D11_INPUT_ELEMENT_DESC layout[] =
{
	{ "POSITION",     0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(SkinnedVertex, position),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",       0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(SkinnedVertex, normal),     D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR",        0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(SkinnedVertex, color),      D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD",     0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(SkinnedVertex, texcoord),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT,  0, offsetof(SkinnedVertex, boneIndex),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(SkinnedVertex, boneWeight), D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
```

---

## 5. 頂点シェーダー出力（VS_OUTPUT）

```hlsl
struct VS_OUTPUT
{
	float4 posH     : SV_POSITION;  // クリップ空間座標（ラスタライザへ）
	float4 color    : COLOR0;       // 頂点カラー（パススルー）
	float2 texcoord : TEXCOORD0;    // UV座標（パススルー）
	float4 normal   : NORMAL0;      // ワールド法線（正規化済み）
	float4 worldPos : TEXCOORD1;    // ワールド座標（ライティング計算用）
};
```

---

## 6. スキニング計算アルゴリズム（Linear Blend Skinning）

### 処理フロー

```
入力頂点（ローカル座標）
  ↓
[ウェイト総和チェック]
  totalWeight = boneWeight.x + .y + .z + .w
  ↓
  totalWeight > 0:  スキニング計算（ブレンド）
  totalWeight == 0: バインドポーズそのまま使用（非スキニングメッシュ互換）
  ↓
[スキニング計算（4ボーン）]
  skinnedPos    += weight[i] * (BoneMatrices[index[i]] × posL)
  skinnedNormal += weight[i] * (BoneMatrices[index[i]] × normalL)
  ↓
  skinnedPos.w = 1.0f  （同次座標の補正）
  ↓
[クリップ座標変換]
  posH     = mul(skinnedPos, mtx)         // WVP 変換
  worldPos = mul(skinnedPos, worldMtx)    // ワールド変換
  normal   = normalize(mul(normal, worldMtx)) // 法線のワールド変換
```

### 重要な実装上の注意点

1. **インデックス範囲チェック**: `idx < MAX_BONES` の条件で必ず範囲確認すること。
2. **ウェイト正規化**: シェーダー内では正規化を行わない。C++ 側（モデルロード時）でウェイトの合計が 1.0 になるよう正規化して渡すこと。
3. **ウェイト = 0 のボーンはスキップ**: `w > 0.0f` チェックで無駄な行列乗算を回避。
4. **法線変換**: 法線は `w = 0.0f` の 4D ベクトルとして変換し、平行移動成分を除外する。
5. **非スキニングメッシュへの互換**: `totalWeight == 0` の場合はそのまま `posL`、`normalL` を使用するため、スキニングなし頂点データと同じシェーダーで描画できる。

---

## 7. ボーン行列の意味と構成

C++ 側で計算されるボーン行列の構成：

```
BoneMatrices[i] = BoneOffsetMatrix[i] × GlobalTransform × GlobalInverseTransform
```

| 行列 | 意味 |
|---|---|
| `BoneOffsetMatrix` | メッシュ空間 → ボーンローカル空間への逆バインドポーズ行列（`aiBone::mOffsetMatrix`） |
| `GlobalTransform` | シーンルートからボーンノードまでのグローバル変換（アニメーション適用済み） |
| `GlobalInverseTransform` | ルートノードのグローバル変換の逆行列（座標系補正） |

シェーダーはこの計算済み行列を受け取るだけでよい。

---

## 8. C++ 側のバインド手順

```cpp
// 1. WVP 行列を b0 にセット
context->VSSetConstantBuffers(0, 1, &pBuffer0);  // mtx（WVP）

// 2. ワールド行列を b1 にセット
context->VSSetConstantBuffers(1, 1, &pBuffer1);  // worldMtx

// 3. ライト情報を b2 にセット（ピクセルシェーダー共用）
context->VSSetConstantBuffers(2, 1, &pBuffer2);  // PointLights + Ambient

// 4. ボーン行列を b5 にセット（毎フレーム更新）
D3D11_MAPPED_SUBRESOURCE mapped;
context->Map(pBoneBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
memcpy(mapped.pData, boneMatrices.matrices, sizeof(XMMATRIX) * MAX_BONES);
context->Unmap(pBoneBuffer, 0);
context->VSSetConstantBuffers(5, 1, &pBoneBuffer);  // BoneMatrices

// 5. スキニング頂点バッファをセット（BLENDINDICES / BLENDWEIGHT 付き）
context->IASetVertexBuffers(0, 1, &pSkinnedVB, &stride, &offset);

// 6. 入力レイアウトをセット（スキニング用）
context->IASetInputLayout(pSkinnedInputLayout);

// 7. 頂点シェーダーをセット
context->VSSetShader(pSkinningVS, nullptr, 0);
```

---

## 9. AnimSprite3D クラスとの整合性

### クラス構造の概要

```
Sprite3D（基底）
  └─ AnimSprite3D（派生）
	   ├─ m_AnimState    : AnimationState   // 再生状態（時間・ループ・クリップへのポインタ）
	   ├─ m_AnimClip     : AnimationClip    // 現在のアニメーションデータ
	   ├─ m_BlendState   : AnimationBlendState  // アニメーション間ブレンド状態
	   └─ m_BoneMatrices : BoneMatrices     // GPU へ渡すボーン行列（256本）
```

### フレームごとの処理フロー

```
毎フレーム Update 内:
  AnimSprite3D::UpdateAnimation(dt)
	├─ [ブレンド中] blendElapsed += dt
	│    └─ blendElapsed >= blendDuration → SetAnimationClip() + PlayAnimation()
	├─ time += dt × tps  （秒 → ティック変換）
	├─ ループ / 停止判定
	└─ UpdateBoneMatrices()
		 ├─ [ブレンド中] 前状態と次状態の行列を lerp（要素ごと線形補間）
		 └─ [通常] CalcBoneMatricesRecursive()
			  ├─ ノード階層を再帰走査
			  ├─ $AssimpFbx$ 分解ノードの二重変換を防ぐ処理
			  ├─ nodeToAnimIndex でアニメーションチャンネルを引く
			  ├─ Trans / Rot / Scale を線形補間・Slerp で補間
			  └─ 最終行列 = Offset × Global × GlobalInverse を格納

毎フレーム Draw 内:
  AnimSprite3D::Draw()
	└─ ModelAnimationDraw(model, pos, rot, scale, m_BoneMatrices, ...)
		 └─ BoneBuffer(b5) に m_BoneMatrices を転送 → DrawIndexed
```

### アニメーション再生 API

| メソッド | 説明 |
|---|---|
| `PlayAnimationByName(name, loop)` | 名前でアニメーションを検索して再生。別アニメ再生中は自動ブレンド遷移（0.3秒）。 |
| `PlayAnimationByIndex(index, loop)` | インデックスで直接再生。ブレンドなし。 |
| `PlayAnimation(loop)` | 設定済みクリップを先頭から再生。 |
| `StopAnimation()` | 停止（time はリセットしない）。 |
| `PauseAnimation()` | 一時停止。 |
| `ResumeAnimation()` | 一時停止から再開。 |
| `SetAnimationBlendDuration(sec)` | ブレンド時間を秒単位で設定（デフォルト 0.3 秒）。 |
| `GetAnimationCount()` | FBX 内のアニメーション数を取得。 |
| `GetAnimationName(index)` | インデックスからアニメーション名を取得。 |

### アニメーション名の検索優先順位（PlayAnimationByName）

1. 完全一致（`strcmp`）
2. 部分一致（`std::string::find`）
3. FBX 内アニメーションが 1 つのみ → 名前不一致でも使用

---

## 10. キーフレームデータ構造

```cpp
struct KeyVec3 { double time; XMFLOAT3 value; };  // 位置・スケール
struct KeyQuat { double time; XMFLOAT4 value; };  // 回転（x, y, z, w）

struct BoneKeyframes {
	std::vector<KeyVec3> trans;   // 平行移動
	std::vector<KeyVec3> scale;   // スケール
	std::vector<KeyQuat> rot;     // 回転
};

struct AnimationClip {
	double duration;              // アニメーション長（ティック単位）
	double tps;                   // Ticks Per Second（デフォルト 24.0）
	std::vector<BoneKeyframes> tracks;  // チャンネル（ノード）ごとのキーフレーム
};
```

### 補間方式

| データ | 補間方式 |
|---|---|
| 位置（Trans） | 線形補間（Lerp） |
| スケール（Scale） | 線形補間（Lerp） |
| 回転（Rot） | 球面線形補間（Slerp / `XMQuaternionSlerp`） |

- 時間が最初のキー以前 → 先頭キーの値を使用
- 時間が最後のキー以降 → 末尾キーの値を使用

---

## 11. Assimp 依存部分と抽象化ポイント

移植先で Assimp を使用しない場合、以下の部分を差し替える必要がある。

| 依存箇所 | 内容 | 代替手段 |
|---|---|---|
| `ModelLoad` 内のボーン収集 | `aiBone::mOffsetMatrix` からオフセット行列取得 | glTF/FBX SDK 等から同等データを取得 |
| `CalcBoneMatricesRecursive` | `aiNode` ツリー再帰走査 | 独自ノード階層構造 + 再帰処理に置換 |
| `ExtractAnimationFromAssimp` | `aiAnimation` → `AnimationClip` 変換 | 他フォーマットのアニメデータを `AnimationClip` 形式に変換 |
| `$AssimpFbx$` ノード処理 | FBX 読み込み時に Assimp が挿入する仮想ノードの除去ロジック | Assimp を使わない場合は不要 |
| `GlobalInverseTransform` | `aiScene->mRootNode->mTransformation` の逆行列 | ルートノードの変換逆行列を別途取得 |

---

## 12. 移植チェックリスト

- [ ] `Common.hlsl`（`POINT_LIGHT` 構造体・マクロ）を用意する
- [ ] `MAX_BONES = 256` を HLSL と C++ の両方で統一する
- [ ] cbuffer レジスタ番号（b0/b1/b2/b5）を両側で一致させる
- [ ] 頂点レイアウトに `BLENDINDICES0`（uint4）と `BLENDWEIGHT0`（float4）を含める
- [ ] スキニング用頂点バッファをモデルロード時に構築し、ボーンインデックス・ウェイトを格納する
- [ ] ボーンウェイトをモデルロード時に正規化する（合計 ≤ 1.0）
- [ ] `BoneBuffer`（b5）を動的バッファ（`D3D11_USAGE_DYNAMIC`）で作成し毎フレーム更新する
- [ ] `BoneOffsetMatrices`・`GlobalInverseTransform`・`BoneNameToIndex` をモデルロード時に収集する
- [ ] `NodeToAnimIndex`（ノード名 → チャンネルインデックス）をアニメーション切り替え時に再構築する
- [ ] `UpdateAnimation(dt)` を毎フレーム呼び出す
- [ ] ブレンド時間のデフォルト（0.3 秒）を要件に合わせて調整する
- [ ] 非スキニングメッシュと同一シェーダーで描画する場合、全ウェイト = 0 で渡すこと

---

## 13. ファイルエンコーディング要件

| ファイル種別 | エンコーディング |
|---|---|
| `.hlsl` シェーダーファイル | **UTF-8 BOM なし**（BOM があるとコンパイルエラー） |
| `.h` / `.cpp` C++ ファイル | **UTF-8 BOM あり** |

---

## 14. 現プロジェクトでの実装結果（2026-06 時点）

本仕様に対して、GravityNotes 側で以下を実装済み。

- `AnimSprite3D::Draw()` から `ModelAnimationDraw()` へ、`m_BoneMatrices` に加えて `m_AnimState.clip` と `m_AnimState.time` を渡すように変更。
- `ModelAnimationDraw()` 側を拡張し、以下の2経路をサポート。
	- スキニングメッシュ: `VertexSkinned` + スキニングVS + `BoneBuffer` 更新で描画。
	- 非スキニングだがノードアニメーションを持つメッシュ: ノード階層再帰で `AnimationClip` と再生時刻からローカルTRSを補間し、CPU側でノード変換を適用して描画。
- スキニング用頂点シェーダーの定数バッファ競合を回避。
	- 既存 `Common.hlsl` の `CameraBuffer` が `b5` を使用しているため、`BoneBuffer` は `b7` に変更。
	- C++ 側の `VSSetConstantBuffers` も `7` スロットに合わせて変更。
- `debug_model_scene` での検証用に `kirbyanim.fbx` の再生切り替え（数字キー）と即時ログ出力を追加。
- ノードアニメーション経路で発生していた色変化（黄ばみ）を修正。
	- `useColorReplace == false` 時は `finalColor = (1,1,1,1)` を使う既存経路のルールに統一し、二重乗算を防止。

### 14-1. 仕様との差分メモ

- 本仕様書の一般例では `BoneBuffer` を `b5` として記載しているが、GravityNotes 実装では既存レジスタ設計との競合回避のため `b7` を使用。
- 移植時は「仕様書の理想値」ではなく「プロジェクト内既存 cbuffer 割当」との整合を優先すること。

---

## 15. 今後の改善案

### 15-1. 設計分離

- 現在 `model.cpp` が `AnimationClip` 補間ロジックの一部を持っているため、アニメーション補間ユーティリティを中立モジュールへ分離する。
- `ModelAnimationDraw()` の責務を整理し、
	- GPUスキニング経路
	- ノードアニメーション経路
	を関数単位で分割して保守性を上げる。

### 15-2. cbuffer 設計の明文化

- `Common.hlsl` と個別VSでのレジスタ使用表を統合ドキュメント化する。
- `b0..bN` の予約方針（World/View/Projection/Material/Light/Camera/Bone など）を固定し、衝突を事前防止する。

### 15-3. アニメーション品質改善

- ボーンウェイトの正規化をロード時に明示的に行い、異常データ時の見た目劣化を抑制する。
- ブレンド処理を行列の要素線形補間から、TRS分解ベース補間（回転はクォータニオン）へ改善して破綻を減らす。

### 15-4. デバッグ支援

- 再生中アニメーション名、インデックス、再生時刻、ループ状態をオンスクリーン表示できるようにする。
- ノードアニメーション経路の有効化状態（GPUスキニング/CPUノード補間）をログまたはUIで可視化する。

### 15-5. 仕様書運用

- 「一般移植要件」と「GravityNotes 実装固有差分」をセクション分離して、今後の改修時に差分追従しやすくする。
- 実装変更時は本仕様書の「14. 実装結果」を更新し、ドキュメントとコードの乖離を防ぐ。
