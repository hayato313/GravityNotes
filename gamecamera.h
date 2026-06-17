#pragma once
#include "player.h"

class GameCamera:public Camera
{
private:
	static GameCamera* s_Instance;

public:
	static void Init();
	static void Update(Player* player);
	static void Draw();
	static void Finalize();
};