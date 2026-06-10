#include "debugscene.h"
#include "debug_model_scene.h"
#include "debug_lighting_scene.h"
#include "debugscore.h"
#include "keyboard.h"

using namespace DirectX;

// デバッグシーンのサブタイプ
enum DEBUG_TYPE {
	DEBUG_MODEL = 0,
	DEBUG_LIGHTING,
	DEBUG_SCORE,
	DEBUG_MAX
};

static DEBUG_TYPE g_type = DEBUG_MODEL;

void DebugScene_Initialize(void)
{
	switch (g_type)
	{
	case DEBUG_MODEL:
		DebugModelScene_Initialize();
		break;
	case DEBUG_LIGHTING:
		DebugLightingScene_Initialize();
		break;
	case DEBUG_SCORE:
		Debugscore_Initialize();
		break;
	default:
		break;
	}
}

void DebugScene_Update(void)
{
	// Tab キーでデバッグシーン切り替え
	if (Keyboard_IsKeyDownTrigger(KK_TAB))
	{
		// 現在のシーンを終了
		switch (g_type)
		{
		case DEBUG_MODEL:
			DebugModelScene_Finalize();
			break;
		case DEBUG_LIGHTING:
			DebugLightingScene_Finalize();
			break;
		case DEBUG_SCORE:
			Debugscore_Finalize();
			break;
		default:
			break;
		}

		// 次のシーンへ
		g_type = (DEBUG_TYPE)((g_type + 1) % DEBUG_MAX);

		// 新しいシーンを初期化
		switch (g_type)
		{
		case DEBUG_MODEL:
			DebugModelScene_Initialize();
			break;
		case DEBUG_LIGHTING:
			DebugLightingScene_Initialize();
			break;
		case DEBUG_SCORE:
			Debugscore_Initialize();
			break;
		default:
			break;
		}
	}

	// 現在のシーンを更新
	switch (g_type)
	{
	case DEBUG_MODEL:
		DebugModelScene_Update();
		break;
	case DEBUG_LIGHTING:
		DebugLightingScene_Update();
		break;
	case DEBUG_SCORE:
		Debugscore_Update();
		break;
	default:
		break;
	}
}

void DebugScene_Draw(void)
{
	switch (g_type)
	{
	case DEBUG_MODEL:
		DebugModelScene_Draw();
		break;
	case DEBUG_LIGHTING:
		DebugLightingScene_Draw();
		break;
	case DEBUG_SCORE:
		Debugscore_Draw();
		break;
	default:
		break;
	}
}

void DebugScene_Finalize(void)
{
	switch (g_type)
	{
	case DEBUG_MODEL:
		DebugModelScene_Finalize();
		break;
	case DEBUG_LIGHTING:
		DebugLightingScene_Finalize();
		break;
	case DEBUG_SCORE:
		Debugscore_Finalize();
		break;
	default:
		break;
	}
}

