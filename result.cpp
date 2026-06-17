#include "result.h"
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
#include "scene.h"

using namespace DirectX;

// ①インスタンス、ポインタ用意
static Sprite2D* g_pResultSprite = nullptr;
static ClickFont* g_pChangeSceneText = nullptr;

void Result_Initialize(void)
{
	// ②各種初期化
	g_pResultSprite = new Sprite2D(
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
		"[result.cpp] タイトルへ"									//テキスト
	);

	UnLockMouse();//マウスアンロック
}

void Result_Update(void)
{
	//③処理
	g_pChangeSceneText->Update();

	//ClickFontがクリックされた
	if (g_pChangeSceneText->IsClick())
	{
		SetPlayJson("");
		SetSceneFade(SCENE_TITLE);
	}
}

void Result_Draw(void)
{
	//④描画
	g_pResultSprite->Draw();
	g_pChangeSceneText->Draw();
}

void Result_Finalize(void)
{
	//⑤解放
	SAFE_DELETE(g_pResultSprite);
	SAFE_DELETE(g_pChangeSceneText);
}
