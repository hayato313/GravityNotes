#pragma once
#include "note_base.h"

class EnemyNote : public NoteBase
{
public:
	EnemyNote() : NoteBase() {}

	void Init(int lane, int face, float spawnZ, float speed);
	void OnHit()  override;
	void OnMiss() override;
};