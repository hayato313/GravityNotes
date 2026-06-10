#include "stageselect.h"
#include "sprite2d.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "font.h"
#include "mouse.h"
#include "sound.h"
#include "ClickFont.h"

using namespace DirectX;

// ①インスタンス、ポインタ用意
static Sprite2D* g_pStageSelectSprite = nullptr;
static ClickFont* g_pChangeSceneText = nullptr;

void StageSelect_Initialize(void)
{
	// ②各種初期化
	g_pStageSelectSprite = new Sprite2D(
		{ SCREEN_WIDTH / 2, SCREEN_HEIGHT / 3 },					//位置
		{ 300.0f, 300.0f },											//サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },									//RGBA
		BLENDSTATE_NONE,											//BlendState
		L"asset\\texture\\tex.png"									//テクスチャパス
	);

	g_pChangeSceneText = new ClickFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 4.0f * 3 },			//位置
		50.0f,														//文字サイズ
		0.0f,														//回転（度）
		{ 1.0f, 1.0f, 1.0f, 1.0f },									//通常色
		{ 1.0f, 0.8f, 0.2f, 1.0f },									//ホバー色
		"[stageselect.cpp] ゲームへ"								//テキスト
	);

	UnLockMouse();//マウスアンロック
}

void StageSelect_Update(void)
{
	//③処理
	g_pChangeSceneText->Update();

	//ClickFontがクリックされた
	if (g_pChangeSceneText->IsClick())
	{
		SetSceneFade(SCENE_GAME);
	}
}

void StageSelect_Draw(void)
{
	//④描画
	g_pStageSelectSprite->Draw();
	g_pChangeSceneText->Draw();
}

void StageSelect_Finalize(void)
{
	//⑤解放
	SAFE_DELETE(g_pStageSelectSprite);
	SAFE_DELETE(g_pChangeSceneText);
}
