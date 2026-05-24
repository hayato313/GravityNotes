#include "debugscore.h"
#include "light.h"
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
#include "sprite3d.h"
#include "ClickFont.h"
#include "camera.h"
#include "movie.h"
#include "debugcamera.h"
#include <string>
#include <cmath>

using namespace DirectX;

Movie* g_pMovie;

void Debugscore_Initialize(void)
{
	g_pMovie = new Movie(
		{ SCREEN_WIDTH / 2,SCREEN_HEIGHT / 2 },
		1000.0f,
		0.0f,
		{ 1.0f,1.0f,1.0f, 1.0f },
		BLENDSTATE_NONE,
		L"asset\\movie\\nullmovie.mp4"
	);
}

void Debugscore_Update(void)
{
	g_pMovie->Update();
}

void Debugscore_Draw(void)
{
	SetDepthEnable(true);


	SetDepthEnable(false);


	g_pMovie->Draw();
}

void Debugscore_Finalize(void)
{
	delete g_pMovie;
	g_pMovie = nullptr;
}
