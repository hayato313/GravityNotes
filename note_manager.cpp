#include "define.h"
#include "game.h"
#include "note_manager.h"
#include "enemy_note.h"
#include "orb_note.h"
#include "barrier_note.h"

static const float HIT_ZONE_Z     = 3.0f;
static const float PASSIVE_ZONE_Z = 0.5f; // Orb・Barrier の自動判定Z
static const float PERFECT_WINDOW = 1.0f;
static const float GOOD_WINDOW    = 2.5f;

// beat を「そのノーツをスポーンすべき時刻（秒）」に変換
float NoteManager::BeatToSpawnTime(float beat) const
{
	float hitTime    = beat * 60.0f / m_ScoreData.bpm;
	float travelTime = (m_SpawnZ - HIT_ZONE_Z) / m_NoteSpeed;
	return hitTime - travelTime;
}

int NoteManager::WallToFace(ScoreWall wall) const
{
	switch (wall)
	{
	case ScoreWall::Down:  return 0; // FACE_FLOOR
	case ScoreWall::Left:  return 1; // FACE_LEFT_WALL
	case ScoreWall::Up:    return 2; // FACE_CEILING
	case ScoreWall::Right: return 3; // FACE_RIGHT_WALL
	default:               return 0;
	}
}

void NoteManager::Init(const std::string& scoreFilePath)
{
	m_NoteSpeed      = 20.0f;
	m_SpawnZ         = 80.0f;
	m_ElapsedTime    = 0.0f;
	m_NextEventIndex = 0;

	m_ScoreData = LoadScore(scoreFilePath);
}

void NoteManager::Update(int playerLane, int playerFace)
{
	m_ElapsedTime += dt;

	// スポーン処理：時刻が来たイベントを順番に生成
	while (m_NextEventIndex < (int)m_ScoreData.events.size())
	{
		const ScoreEvent& ev = m_ScoreData.events[m_NextEventIndex];
		if (m_ElapsedTime < BeatToSpawnTime(ev.beat)) break;

		int face = WallToFace(ev.wall);

		// 現時刻でノーツが居るべきZ座標を計算（遅延スポーン時は手前に補正）
		float hitTime = ev.beat * 60.0f / m_ScoreData.bpm;
		float initZ   = (hitTime - m_ElapsedTime) * m_NoteSpeed + HIT_ZONE_Z;

		switch (ev.type)
		{
		case ScoreType::Enemy:
		{
			EnemyNote* note = new EnemyNote();
			note->Init(ev.lane, face, initZ, m_NoteSpeed);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::Orb:
		{
			OrbNote* note = new OrbNote();
			note->Init(ev.lane, face, initZ, m_NoteSpeed);
			m_Notes.push_back(note);
			break;
		}
		case ScoreType::Barrier:
		{
			BarrierNote* note = new BarrierNote();
			note->Init(ev.lane, face, initZ, m_NoteSpeed);
			m_Notes.push_back(note);
			break;
		}
		default:
			break;
		}
		m_NextEventIndex++;
	}

	// 更新・自動判定・削除
	for (int i = (int)m_Notes.size() - 1; i >= 0; i--)
	{
		m_Notes[i]->Update();

		if (!m_Notes[i]->IsHit())
		{
			float z = m_Notes[i]->GetPosZ();

			// Orb: 同 lane・face なら自動取得、違えばMiss
			if (OrbNote* orb = dynamic_cast<OrbNote*>(m_Notes[i]))
			{
				if (z <= PASSIVE_ZONE_Z)
				{
					if (m_Notes[i]->GetLaneIndex() == playerLane &&
						m_Notes[i]->GetFace()      == playerFace)
						orb->OnHit();
					else
						orb->OnMiss();
				}
			}
			// Barrier: 同 lane・face なら被弾、違えば回避成功
			else if (BarrierNote* barrier = dynamic_cast<BarrierNote*>(m_Notes[i]))
			{
				if (z <= PASSIVE_ZONE_Z)
				{
					if (m_Notes[i]->GetLaneIndex() == playerLane &&
						m_Notes[i]->GetFace()      == playerFace)
						barrier->OnMiss();
					else
						barrier->OnHit();
				}
			}
			// Enemy: 判定窓を通過したら押し逃しMiss
			else if (z < HIT_ZONE_Z - GOOD_WINDOW)
			{
				m_Notes[i]->OnMiss();
			}
		}

		if (!m_Notes[i]->IsActive())
		{
			delete m_Notes[i];
			m_Notes.erase(m_Notes.begin() + i);
		}
	}
}

void NoteManager::Draw()
{
	for (NoteBase* note : m_Notes)
		note->Draw();
}

void NoteManager::Finalize()
{
	for (NoteBase* note : m_Notes)
		delete note;
	m_Notes.clear();
}

JUDGE NoteManager::Judge(int lane, int face)
{
	NoteBase* bestNote = nullptr;
	float bestDist = FLT_MAX;

	for (NoteBase* note : m_Notes)
	{
		if (!note->IsActive() || note->IsHit()) continue;
		if (note->GetLaneIndex() != lane || note->GetFace() != face) continue;
		// Orb・Barrier はボタン判定しない
		if (dynamic_cast<OrbNote*>(note) || dynamic_cast<BarrierNote*>(note)) continue;

		float dist = fabsf(note->GetPosZ() - HIT_ZONE_Z);
		if (dist < bestDist)
		{
			bestDist = dist;
			bestNote = note;
		}
	}

	if (!bestNote) return JUDGE_MISS;

	if (bestDist < PERFECT_WINDOW) { bestNote->OnHit(); return JUDGE_PERFECT; }
	if (bestDist < GOOD_WINDOW)    { bestNote->OnHit(); return JUDGE_GOOD; }
	return JUDGE_MISS;
}