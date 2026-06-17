# FontRenderer 使い方

## 概要
FontRenderer は単一行テキスト描画用の基本クラスです。
UTF-8 文字列を描画し、色変更と事前グリフキャッシュに対応します。

## 主な API
- コンストラクタ
  - FontRenderer(XMFLOAT2 pos, float fontSize, float rotation, XMFLOAT4 color, const std::string& text)
- 描画
  - Draw()
- テキスト変更
  - SetText(const std::string& text)
- 色変更
  - SetColor(XMFLOAT4 color)
- 事前キャッシュ
  - PreCacheGlyphs()

## 最小サンプル
```cpp
#include "font.h"

static FontRenderer* g_pFont = nullptr;

void Sample_Initialize()
{
	g_pFont = new FontRenderer(
		{ SCREEN_WIDTH * 0.5f, SCREEN_HEIGHT * 0.5f }, // 表示位置（画面中央）
		36.0f,                                         // フォントサイズ
		0.0f,                                          // 回転角（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },                    // 文字色 RGBA
		"Hello FontRenderer"                          // 初期テキスト
	);
	g_pFont->PreCacheGlyphs();
}

void Sample_Update()
{
	g_pFont->SetText("Score: 12345");
}

void Sample_Draw()
{
	g_pFont->Draw();
}

void Sample_Finalize()
{
	SAFE_DELETE(g_pFont);
}
```

## 注意点
- 改行は想定していません。複数行は MultiLineFontRenderer を使用してください。
- 座標は SCREEN_WIDTH/SCREEN_HEIGHT 基準で指定してください。
- SetColor はアトラステクスチャ更新を伴うため、毎フレーム連打は避けてください。
