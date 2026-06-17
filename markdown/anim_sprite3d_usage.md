# AnimSprite3D 使い方メモ

## 概要

`AnimSprite3D` は `Sprite3D` を継承した 3D モデル描画クラスで、FBX / GLB に含まれるスキニングアニメーションを再生できる。
このプロジェクトでは `framework/anim_sprite3d.h` と `framework/anim_sprite3d.cpp` に実装がある。

---

## 基本の生成方法

```cpp
AnimSprite3D* player = new AnimSprite3D(
	{ 0.0f, 0.0f, 0.0f },
	{ 1.0f, 1.0f, 1.0f },
	{ 0.0f, 180.0f, 0.0f },
	"asset\\model\\kirbyanim.fbx",
	S_PHONG
);
```

### 引数

1. 位置 `XMFLOAT3`
2. スケール `XMFLOAT3`
3. 回転 `XMFLOAT3`（度数法）
4. モデルパス
5. シェーダー種別

既存コードでは `debugscene/debug_model_scene.cpp` で `asset\\model\\kirbyanim.fbx` を読み込んでいる。

---

## 初期化直後に行うこと

```cpp
player->SetAnimationBlendDuration(0.2);
```

- アニメーション切り替え時のブレンド秒数を設定する。
- 既存実装では 0.2 秒が使われている。

必要なら読み込み直後にアニメーション数を確認できる。

```cpp
unsigned int animCount = player->GetAnimationCount();
```

---

## アニメーション再生

### 名前指定で再生

```cpp
player->PlayAnimationByName("Walk", true);
```

- 第 1 引数: アニメーション名
- 第 2 引数: ループ有無
- 内部では完全一致を優先し、見つからなければ部分一致でも検索する。
- モデル内のアニメーションが 1 つだけなら、名前不一致でもその 1 件を再生する。

### インデックス指定で再生

```cpp
player->PlayAnimationByIndex(0, true);
```

- アニメーション名が不明な場合のフォールバックに使いやすい。

---

## アニメーション一覧の確認

```cpp
unsigned int animCount = player->GetAnimationCount();
for (unsigned int i = 0; i < animCount; i++)
{
	const char* animName = player->GetAnimationName(i);
}
```

- `GetAnimationName` は `nullptr` を返す場合があるため、ログ出力時は null チェックを入れる。
- どの名前で `PlayAnimationByName` を呼ぶべきか不明なときは、先に列挙して確認する。

---

## 毎フレーム更新

```cpp
player->UpdateAnimation(1.0f / 60.0f);
```

- このプロジェクトの既存デバッグ実装では固定値 `1.0f / 60.0f` を使っている。
- `UpdateAnimation` の内部で再生時間更新とボーン行列更新まで行われるため、通常は別途 `UpdateBoneMatrices()` を呼ばなくてよい。

---

## 描画

```cpp
SetDepthEnable(true);
player->Draw();
```

- 3D 描画区間で呼ぶ。
- ライトを使う場合は先にライト適用を済ませておく。

---

## 終了処理

```cpp
delete player;
player = nullptr;
```

- `AnimSprite3D` は `new` で生成しているため、終了時は `delete` で破棄する。

---

## 最小構成の流れ

```cpp
AnimSprite3D* player = nullptr;

player = new AnimSprite3D(
	{ 0.0f, 0.0f, 0.0f },
	{ 1.0f, 1.0f, 1.0f },
	{ 0.0f, 180.0f, 0.0f },
	"asset\\model\\kirbyanim.fbx",
	S_PHONG
);
player->SetAnimationBlendDuration(0.2);

if (!player->PlayAnimationByName("Walk", true))
{
	if (player->GetAnimationCount() > 0)
	{
		player->PlayAnimationByIndex(0, true);
	}
}

player->UpdateAnimation(1.0f / 60.0f);
player->Draw();

delete player;
player = nullptr;
```

---

## このプロジェクトでの注意点

- 使用例は `debugscene/debug_model_scene.cpp` を参照する。
- アニメーション名はモデルファイル依存なので、固定名を使う場合は `GetAnimationName()` で先に確認する。
- モデルがスキニング非対応、またはアニメーション未内包の場合は再生できない。
- `shader/shader.cpp` はこのワークスペースには存在しないため、描画実装確認時は `shader/renderer.cpp` を参照する。
