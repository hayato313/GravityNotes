/*==============================================================================

   ポリゴン描画 [game.h]
--------------------------------------------------------------------------------

==============================================================================*/
#pragma once

#include <d3d11.h>
#include "define.h"

//ゲームシーンにおける定数定義
#define dt	(1.0f/FPS)//
#define LANE_WIDTH	(2.5f)//レーンの間隔

void Game_Initialize(void);
void Game_Finalize(void);
void Game_Update(void);
void Game_Draw(void);
