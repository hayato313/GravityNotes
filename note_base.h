#pragma once
#include "sprite3d.h"

class NoteBase : public Sprite3D
{
protected:
	int   m_LaneIndex;
	int   m_Face;
	float m_Speed;
	bool  m_IsActive;
	bool  m_IsHit;

public:
	NoteBase() : Sprite3D(), m_LaneIndex(0), m_Face(0), m_Speed(0.0f), m_IsActive(false), m_IsHit(false) {}
	virtual ~NoteBase() = default;

	virtual void Init(int lane, int face, float spawnZ, float speed, const char* modelPath);
	virtual void Update();
	virtual void Draw();
	virtual void Finalize();
	virtual void OnHit();
	virtual void OnMiss();

	bool IsActive() const { return m_IsActive; }
	bool IsHit()    const { return m_IsHit; }
	int  GetLaneIndex() const { return m_LaneIndex; }
	int  GetFace()      const { return m_Face; }
};