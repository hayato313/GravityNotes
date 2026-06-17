#include "camera.h"
#include "keyboard.h"
#include "mouse.h"
#include "scene.h"
#include "fade.h"

#include "gamecamera.h"

GameCamera* GameCamera::s_Instance=nullptr;

bool cameraIndex;

void GameCamera::Init() {
	s_Instance = new GameCamera();
	Camera_Initialize();
	LockMouse();
	s_Instance->m_Pos = { 0.0f,0.0f,0.0f };
	s_Instance->m_yaw = 0.0f;
	s_Instance->m_pitch = 0.0f;

	cameraIndex = true;
}

void GameCamera::Update(Player* player) {
	if (Keyboard_IsKeyDownTrigger(KK_D4))cameraIndex= !cameraIndex;

	if (!cameraIndex)
	{
		const float SPEED = 0.1f;
		Mouse_State mouseState;
		Mouse_GetState(&mouseState);

		s_Instance->m_yaw += static_cast<float>(mouseState.x) * 0.1f;
		s_Instance->m_pitch += static_cast<float>(mouseState.y) * 0.1f;

		if (s_Instance->m_pitch > 89.0f) s_Instance->m_pitch = 89.0f;
		if (s_Instance->m_pitch < -89.0f) s_Instance->m_pitch = -89.0f;

		float yawRad = XMConvertToRadians(s_Instance->m_yaw);
		float pitchRad = XMConvertToRadians(s_Instance->m_pitch);

		XMVECTOR forward = XMVectorSet(sinf(yawRad), 0.0f, cosf(yawRad), 0.0f);
		XMVECTOR right = XMVectorSet(cosf(yawRad), 0.0f, -sinf(yawRad), 0.0f);
		XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		XMVECTOR moveDir = XMVectorZero();

		if (Keyboard_IsKeyDown(KK_W)) moveDir = XMVectorAdd(moveDir, forward);
		if (Keyboard_IsKeyDown(KK_S)) moveDir = XMVectorSubtract(moveDir, forward);
		if (Keyboard_IsKeyDown(KK_D)) moveDir = XMVectorAdd(moveDir, right);
		if (Keyboard_IsKeyDown(KK_A)) moveDir = XMVectorSubtract(moveDir, right);

		if (Keyboard_IsKeyDown(KK_SPACE)) moveDir = XMVectorAdd(moveDir, up);
		if (Keyboard_IsKeyDown(KK_LEFTSHIFT) || Keyboard_IsKeyDown(KK_RIGHTSHIFT)) moveDir = XMVectorSubtract(moveDir, up);

		if (!XMVector3Equal(moveDir, XMVectorZero()))
		{
			moveDir = XMVector3Normalize(moveDir);
			moveDir = XMVectorScale(moveDir, SPEED);

			XMVECTOR pos = XMLoadFloat3(&s_Instance->m_Pos);
			pos = XMVectorAdd(pos, moveDir);
			XMStoreFloat3(&s_Instance->m_Pos, pos);
		}

		XMVECTOR lookDir = XMVectorSet(
			sinf(yawRad) * cosf(pitchRad),
			-sinf(pitchRad),
			cosf(yawRad) * cosf(pitchRad),
			0.0f
		);

		XMVECTOR posVec = XMLoadFloat3(&s_Instance->m_Pos);
		XMVECTOR atVec = XMVectorAdd(posVec, lookDir);

		XMFLOAT3 atPos;
		XMStoreFloat3(&atPos, atVec);

		if (GetCamera()) {
			GetCamera()->UpdateView(s_Instance->m_Pos, atPos);
		}
	}
	else {
		//s_Instance->m_Pos = player->GetPos();
		s_Instance->m_Pos = { 0.0f,0.0f,0.0f };
		s_Instance->m_AtPos = { 0.0f,0.0f,10.0f };
	}

	if (Keyboard_IsKeyDownTrigger(KK_ESCAPE))
	{
		SetSceneFade(SCENE_TITLE);
	}
}

void GameCamera::Draw() {

}

void GameCamera::Finalize() {
	Camera_Finalize();
}