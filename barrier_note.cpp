#include "game.h"
#include "barrier_note.h"

void BarrierNote::Init(int lane, int face, float spawnZ, float speed)
{
	NoteBase::Init(lane, face, spawnZ, speed, "asset/model/cube.fbx");
	SetColor(0.0f, 1.0f, 0.0f);
}

void BarrierNote::OnHit()
{
	NoteBase::OnHit();
	// TODO: スコア加算・コンボ継続
}

void BarrierNote::OnMiss()
{
	NoteBase::OnMiss();
	// TODO: HP減少・コンボリセット
}