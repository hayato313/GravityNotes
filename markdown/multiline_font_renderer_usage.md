# MultiLineFontRenderer 使い方

## 概要
MultiLineFontRenderer は FontRenderer を継承した複数行描画クラスです。
テキスト中の \n で行分割し、行間設定と行矩形取得に対応します。

## 主な API
- コンストラクタ
  - MultiLineFontRenderer(XMFLOAT2 pos, float fontSize, float rotation, XMFLOAT4 color, const std::string& text, float lineSpacing = 1.5f)
- 描画
  - Draw()
- テキスト変更
  - SetText(const std::string& text)
- 行間設定
  - SetLineSpacing(float lineSpacing)
  - GetLineSpacing()
- 行矩形取得
  - GetLineRects()

## 最小サンプル
```cpp
#include "MultiLineFontRenderer.h"

static MultiLineFontRenderer* g_pMulti = nullptr;

void Sample_Initialize()
{
	g_pMulti = new MultiLineFontRenderer(
		{ SCREEN_WIDTH * 0.5f, 140.0f },                  // 表示基準位置
		30.0f,                                            // フォントサイズ
		0.0f,                                             // 回転角（度）
		{ 0.9f, 0.95f, 1.0f, 1.0f },                      // 文字色 RGBA
		"1行目: 操作説明\n2行目: Enterで決定\n3行目: Escで戻る", // 初期テキスト（\nで改行）
		1.4f                                              // 行間倍率
	);
}

void Sample_Update()
{
	g_pMulti->SetText("A\nB\nC");
}

void Sample_Draw()
{
	g_pMulti->Draw();
}

void Sample_Finalize()
{
	SAFE_DELETE(g_pMulti);
}
```

## 注意点
- 自動折り返しはありません。改行は \n を明示してください。
- GetLineRects の矩形は行単位判定に利用できます。
- 描画座標の基準は SCREEN_WIDTH/SCREEN_HEIGHT です。
