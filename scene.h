#pragma once

#include <string>

enum SCENE {
	SCENE_TITLE = 0,
	SCENE_STAGESELECT,
	SCENE_GAME,
	SCENE_RESULT,
	SCENE_DEBUG,
	SCENE_MAX,
	SCENE_NONE,
};

void Init(void);
void Update(void);
void Draw(void);
void Finalize(void);

void SetScene(SCENE id);
SCENE GetScene(void);
void SetPlayJson(const std::string& jsonName);
const std::string& GetPlayJson(void);
