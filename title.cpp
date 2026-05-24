#include "title.h"
#include "sprite.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"

#include "renderer.h"
#include "font.h"
#include "mouse.h"
#include "sound.h"
#include "ClickFont.h"
#include "movie.h"
#include <string>
#include <cmath>

using namespace DirectX;

// ①Spriteのインスタンス、ポインタ用意
static Sprite* g_pTitleSprite = nullptr;
static FontRenderer* g_pTitletext = nullptr;
static ClickFont* g_pModelViewerClickFont = nullptr;
static ClickFont* g_pLightingViewerClickFont = nullptr;

void Title_Initialize(void)
{
	// ②各種初期化
	g_pTitleSprite = new Sprite(
		{ SCREEN_WIDTH / 2 - 200.0f, SCREEN_HEIGHT / 2.0f - 100.0f },	//位置
		{ 500.0f, 500.0f },												//サイズ
		0.0f,															//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },										//RGBA
		BLENDSTATE_NONE,												//BlendState
		L"asset\\texture\\tex.png"										//テクスチャパス
	);

	g_pTitletext = new FontRenderer(
		{ SCREEN_WIDTH / 2 - 200.0f, SCREEN_HEIGHT / 2.0f - 100.0f },	//位置
		30.0f,															//文字サイズ
		0.0f,															//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },										//RGBA
		"title.cpp"														//テキスト
	);

	// モデルビューワシーンへのClickFont（左上）
	g_pModelViewerClickFont = new ClickFont(
		{ 0.0f,0.0f },
		22.0f,
		0.0f,
		{ 0.5f, 0.5f, 0.5f, 1.0f },
		{ 0.3f, 0.9f, 1.0f, 1.0f },
		"[Debug] ModelViewer"
	);
	g_pModelViewerClickFont->SetHitSize({ 260.0f, 30.0f });
	g_pModelViewerClickFont->SetOnClick([]() {
		StartFade(SCENE_DEBUG_MODEL);
		});

	// ライティング確認シーンへのClickFont
	g_pLightingViewerClickFont = new ClickFont(
		{ 0.0f,30.0f },
		22.0f,
		0.0f,
		{ 0.7f, 0.7f, 0.7f, 1.0f },
		{ 1.0f, 0.8f, 0.2f, 1.0f },
		"[Debug] Lighting"
	);
	g_pLightingViewerClickFont->SetHitSize({ 260.0f, 30.0f });
	g_pLightingViewerClickFont->SetOnClick([]() {
		StartFade(SCENE_DEBUG_LIGHTING);
		});

	UnLockMouse();//マウスロック
	UnLockMouse();//マウスロック
}

void Title_Update(void)
{

	// ③適当な処理　アニメーションなどもここで
	if (g_pModelViewerClickFont)
	{
		g_pModelViewerClickFont->Update();
	}
	if (g_pLightingViewerClickFont)
	{
		g_pLightingViewerClickFont->Update();
	}
}

void Title_Draw(void)
{
	g_pTitleSprite->Draw();
	g_pModelViewerClickFont->Draw();
	g_pLightingViewerClickFont->Draw();
	g_pTitletext->Draw();
}

void Title_Finalize(void)
{
	delete g_pTitleSprite;
	g_pTitleSprite = nullptr;

	delete g_pModelViewerClickFont;
	g_pModelViewerClickFont = nullptr;

	delete g_pLightingViewerClickFont;
	g_pLightingViewerClickFont = nullptr;

	delete g_pTitletext;
	g_pTitletext = nullptr;
}
