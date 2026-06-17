#include "model.h"

#include "game.h"
#include "note_base.h"

static const float NOTE_TUNNEL_HALF = 2.5f;
static const float NOTE_DESPAWN_Z   = -5.0f;

void NoteBase::Init(int lane, int face, float spawnZ, float speed, const char* modelPath)
{
	m_LaneIndex = lane;
	m_Face      = face;
	m_Speed     = speed;
	m_IsActive  = true;
	m_IsHit     = false;

	// 初回のみモデルをロード（プール再利用時はスキップ）
	if (!m_Model && modelPath)
		m_Model = ModelLoad(modelPath);

	// 面とレーンから初期座標を計算
	// face: 0=FLOOR, 1=LEFT_WALL, 2=CEILING, 3=RIGHT_WALL
	float laneVal = lane * LANE_WIDTH;
	XMFLOAT3 pos = { 0.0f, 0.0f, spawnZ };
	switch (face)
	{
	case 0: pos.x =  laneVal;           pos.y = -NOTE_TUNNEL_HALF; break;
	case 1: pos.x = -NOTE_TUNNEL_HALF;  pos.y =  laneVal;          break;
	case 2: pos.x =  laneVal;           pos.y =  NOTE_TUNNEL_HALF; break;
	case 3: pos.x =  NOTE_TUNNEL_HALF;  pos.y =  laneVal;          break;
	}
	SetPos(pos);
	SetRot({ 0.0f, 0.0f, 0.0f });
	SetSize({ 1.0f, 1.0f, 1.0f });
}

void NoteBase::Update()
{
	AddPosZ(-m_Speed * dt);
	if (GetPosZ() < NOTE_DESPAWN_Z)
		m_IsActive = false;
}

void NoteBase::Draw()
{
	Sprite3D::Draw();
}

void NoteBase::Finalize()
{
	m_IsActive = false;
}

void NoteBase::OnHit()
{
	m_IsHit    = true;
	m_IsActive = false;
}

void NoteBase::OnMiss()
{
	m_IsActive = false;
}