#include "game.h"
#include "orb_note.h"

void OrbNote::Init(int lane, int face, float spawnZ, float speed)
{
	NoteBase::Init(lane, face, spawnZ, speed, "asset/model/circle.fbx");
	SetColor(0.0f, 0.0f, 1.0f);
}

void OrbNote::OnHit()
{
	NoteBase::OnHit();
	// TODO: HP回復・スコア加算
}

void OrbNote::OnMiss()
{
	NoteBase::OnMiss();
	// TODO: コンボリセット
}