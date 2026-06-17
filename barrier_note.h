#pragma once
#include "note_base.h"

class BarrierNote : public NoteBase
{
public:
	BarrierNote() : NoteBase() {}

	void Init(int lane, int face, float spawnZ, float speed);
	void OnHit()  override; // 回避成功
	void OnMiss() override; // 回避失敗（被弾）
};