#include "game.h"
#include "enemy_note.h"

void EnemyNote::Init(int lane, int face, float spawnZ, float speed)
{
	NoteBase::Init(lane, face, spawnZ, speed, "asset/model/enban.fbx");
	SetColor(1.0f, 0.0f, 0.0f);
}

void EnemyNote::OnHit()
{
	NoteBase::OnHit();
	// TODO: スコア加算・コンボ継続
}

void EnemyNote::OnMiss()
{
	NoteBase::OnMiss();
	// TODO: HP減少・コンボリセット
}