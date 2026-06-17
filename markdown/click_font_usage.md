# ClickFont 使い方

## 概要
ClickFont は FontRenderer を継承したクリック可能テキストです。
ホバー色切り替えとクリック検知を 1 行テキスト向けに提供します。

## 主な API
- コンストラクタ
  - ClickFont(XMFLOAT2 pos, float fontSize, float rotation, XMFLOAT4 normalColor, XMFLOAT4 hoverColor, const std::string& text)
- 更新
  - Update()
- 描画
  - Draw()
- 状態取得
  - IsHover()
  - IsClick()
- 当たり判定サイズ調整
  - SetHitSize(XMFLOAT2 size)

## 最小サンプル
```cpp
#include "ClickFont.h"

static ClickFont* g_pStartText = nullptr;

void Sample_Initialize()
{
	g_pStartText = new ClickFont(
		{ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.75f }, // 表示位置
		48.0f,                                          // フォントサイズ
		0.0f,                                           // 回転角（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },                     // 通常時カラー
		{ 1.0f, 0.8f, 0.2f, 1.0f },                     // ホバー時カラー
		"Game Start"                                   // 表示テキスト
	);
}

void Sample_Update()
{
	g_pStartText->Update();
	if (g_pStartText->IsClick())
	{
		SetSceneFade(SCENE_GAME);
	}
}

void Sample_Draw()
{
	g_pStartText->Draw();
}

void Sample_Finalize()
{
	SAFE_DELETE(g_pStartText);
}
```

## 注意点
- クリック判定は 1 つの矩形で行われます。
- 複数行の行単位クリックが必要な場合は MultiLineClickFont を使用してください。
- クリック判定は論理座標に変換して行うため、ウィンドウサイズ変更時でも動作します。
