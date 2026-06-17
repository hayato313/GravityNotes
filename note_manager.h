#pragma once
#include <vector>
#include "note_base.h"
#include "scoreloader.h"

enum JUDGE {
	JUDGE_PERFECT,
	JUDGE_GOOD,
	JUDGE_MISS
};

class NoteManager
{
private:
	std::vector<NoteBase*> m_Notes;
	float    m_NoteSpeed;
	float    m_SpawnZ;

	ScoreData m_ScoreData;
	float     m_ElapsedTime;
	int       m_NextEventIndex;

	float BeatToSpawnTime(float beat) const;
	int   WallToFace(ScoreWall wall)  const;

public:
	void  Init(const std::string& scoreFilePath);
	void  Update(int playerLane, int playerFace);
	void  Draw();
	void  Finalize();

	JUDGE Judge(int lane, int face);
};