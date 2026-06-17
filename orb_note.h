#pragma once
#include "note_base.h"

class OrbNote : public NoteBase
{
public:
	OrbNote() : NoteBase() {}

	void Init(int lane, int face, float spawnZ, float speed);
	void OnHit()  override;
	void OnMiss() override;
};