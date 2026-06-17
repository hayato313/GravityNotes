#include "debugrigplayer.h"
#include "light.h"
#include "sprite2d.h"
#include "texture.h"
#include "keyboard.h"
#include "fade.h"
#include "debug_ostream.h"
#include "define.h"
#include "renderer.h"
#include "font.h"
#include "mouse.h"
#include "sound.h"
#include "sprite3d.h"
#include "anim_sprite3d.h"
#include "ClickFont.h"
#include "camera.h"
#include "movie.h"
#include "debugcamera.h"
#include <string>
#include <cmath>

using namespace DirectX;

static AnimSprite3D* g_pRigPlayer = nullptr;
static PointLight* g_pMainLight = nullptr;
static AmbientLight g_AmbientLight(XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f));
static FontRenderer* g_pGuideFont = nullptr;
static FontRenderer* g_pAnimInfoFont = nullptr;

static int GetTriggeredAnimationSlot()
{
	static const Keyboard_Keys keys[] = {
		KK_D1,
		KK_D2,
		KK_D3,
		KK_D4,
		KK_D5,
		KK_D6,
		KK_D7,
		KK_D8,
		KK_D9,
		KK_D0,
	};

	for (int i = 0; i < (int)ARRAYSIZE(keys); i++)
	{
		if (Keyboard_IsKeyDownTrigger(keys[i]))
		{
			return i;
		}
	}

	return -1;
}

void DebugRigPlayer_Initialize(void)
{
	g_pRigPlayer = new AnimSprite3D(
		{ 0.0f, -6.0f, 5.0f },
		{ 1.0f, 1.0f, 1.0f },
		{ 0.0f, 180.0f, 0.0f },
		"asset\\model\\kirbyanim.fbx",
		S_PHONG
	);

	if (g_pRigPlayer)
	{
		g_pRigPlayer->SetAnimationBlendDuration(0.2);
		unsigned int animCount = g_pRigPlayer->GetAnimationCount();
		if (animCount > 0)
		{
			g_pRigPlayer->PlayAnimationByIndex(0, true);
			g_pRigPlayer->UpdateAnimation(0.0f);
		}
	}

	g_pMainLight = new PointLight(
		TRUE,
		{ 0.0f, 5.0f, -5.0f, 1.0f },
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		20.0f,
		1.0f
	);
	g_pMainLight->Apply(g_AmbientLight);

	DebugCamera_Initialize();
	if (GetCamera())
	{
		GetCamera()->SetTargetPos({ 0.0f, -3.0f, 5.0f });
		SetCameraPosition(GetCamera()->GetPos());
	}

	g_pGuideFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, 30.0f },
		22.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		"WASD/SPACE/SHIFT:Camera  1-0:Animation"
	);
	g_pGuideFont->PreCacheGlyphs();

	g_pAnimInfoFont = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT - 40.0f },
		22.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"Animation: none"
	);
	g_pAnimInfoFont->PreCacheGlyphs();
}

void DebugRigPlayer_Update(void)
{
	DebugCamera_Update();
	if (GetCamera())
	{
		GetCamera()->SetTargetPos({ 0.0f, -3.0f, 5.0f });
		SetCameraPosition(GetCamera()->GetPos());
	}

	if (g_pMainLight)
	{
		g_pMainLight->Apply(g_AmbientLight);
	}

	if (g_pRigPlayer)
	{
		int animSlot = GetTriggeredAnimationSlot();
		unsigned int animCount = g_pRigPlayer->GetAnimationCount();
		if (animSlot >= 0 && (unsigned int)animSlot < animCount)
		{
			if (g_pRigPlayer->PlayAnimationByIndex((unsigned int)animSlot, true))
			{
				g_pRigPlayer->UpdateAnimation(0.0f);
				const char* animName = g_pRigPlayer->GetAnimationName((unsigned int)animSlot);
				if (g_pAnimInfoFont)
				{
					std::string text = "Animation: ";
					text += (animName && animName[0] != '\0') ? animName : "<noname>";
					g_pAnimInfoFont->SetText(text);
					g_pAnimInfoFont->PreCacheGlyphs();
				}
			}
		}

		if (g_pAnimInfoFont && animCount == 0)
		{
			g_pAnimInfoFont->SetText("Animation: not found");
		}

		g_pRigPlayer->UpdateAnimation(1.0f / 60.0f);
	}
}

void DebugRigPlayer_Draw(void)
{
	SetDepthEnable(true);
	if (g_pRigPlayer)
	{
		g_pRigPlayer->Draw();
	}

	SetDepthEnable(false);
	Sprite_BeginDraw2D();
	if (g_pGuideFont)
	{
		g_pGuideFont->Draw();
	}
	if (g_pAnimInfoFont)
	{
		g_pAnimInfoFont->Draw();
	}
	DebugCamera_Draw();
	Sprite_EndDraw2D();
}

void DebugRigPlayer_Finalize(void)
{
	delete g_pRigPlayer;
	g_pRigPlayer = nullptr;

	delete g_pMainLight;
	g_pMainLight = nullptr;

	delete g_pGuideFont;
	g_pGuideFont = nullptr;

	delete g_pAnimInfoFont;
	g_pAnimInfoFont = nullptr;

	DebugCamera_Finalize();
}
