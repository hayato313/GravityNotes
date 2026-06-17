#include "field.h"
#include "model.h"
#include "shadermanager.h"
#include "define.h"
#include "debug_params.h"
#include <cstdio>

using namespace DirectX;

void Field::Init() {
	m_Position = { 0.0f,0.0f,10.0f };
	m_Color = { 1.0f,1.0f,1.0f,1.0f };
}

void Field::Update(){

}

void Field::Draw() {
	if (m_Model) {
		ModelDraw(
			m_Model,
			m_Position,
			m_Rotation,
			m_Scale,
			m_Color,
			false,
			S_LAMBERT
		);
	}
}

void Field::Finalize() {
}