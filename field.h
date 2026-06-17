#pragma once
#include "renderer.h"
#include "sprite3d.h"

struct MODEL;

class Field: public Sprite3D
{
private:


public:
	// デフォルトコンストラクタを追加し、基底 Sprite3D のコンストラクタを呼ぶ
	Field()
		: Sprite3D(XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), 
			"asset/model/tunnel_segment.fbx", S_UNLIT)
	{}
    void Init();
    void Update();
    void Draw();
    void Finalize();
};
