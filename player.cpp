#include "keyboard.h"
#include "mouse.h"
#include "gamepad.h"
#include "mathhelper.h"

#include "game.h"
#include "note_manager.h"
#include "player.h"

int pad;

void Player::Init(NoteManager* nm)
{
	m_pNoteManager = nm;
	pad = Gamepad_FindConnectedPlayer();//現在仮置き、多分フレームワークに置いた方が各シーンで使いやすい
	m_GravityFace = FACE::FACE_FLOOR;
	m_LaneIndex = LANE_CENTER;
	m_TargetLaneIndex = LANE_CENTER;
	m_IsMoving = false;
	m_MoveTimer = 0.0f;
	m_MoveDuration = 0.12f;

	m_IsGravityMoving = false;
	m_GravityTimer = 0.0f;
	m_GravityDuration = 0.3f;

	m_Position = { 0.0f,-2.5f,0.0f };
	m_StartPos = m_Position;
	m_TargetPos = m_Position;
	m_Rotation = { 0.0f,180.0f,0.0f };
	m_GravityStartPos = m_Position;
	m_GravityStartRot = m_Rotation;
}

void Player::Update()
{
	//lane移動入力
	if (m_GravityFace == FACE::FACE_FLOOR || m_GravityFace == FACE::FACE_CEILING)
	{
		if (Gamepad_GetLeftStick(pad).x < 0 || Keyboard_IsKeyDownTrigger(KK_A))//左移動
			MoveLeft();
		else if (Gamepad_GetLeftStick(pad).x > 0 || Keyboard_IsKeyDownTrigger(KK_D))//右移動
			MoveRight();
	}
	else
	{
		if (Gamepad_GetLeftStick(pad).y > 0 || Keyboard_IsKeyDownTrigger(KK_W))//上移動
			MoveRight();
		else if (Gamepad_GetLeftStick(pad).y < 0 || Keyboard_IsKeyDownTrigger(KK_S))//下移動
			MoveLeft();
	}

	//移動補間
	if (m_IsMoving)
	{
		m_MoveTimer += dt;
		float t = m_MoveTimer / m_MoveDuration;
		if (t >= 1.0f)
		{
			t = 1.0f;
			m_LaneIndex = m_TargetLaneIndex;
			m_IsMoving = false;
		}
		float eased = 1.0f - (1.0f - t) * (1.0f - t);
		m_Position.x = m_StartPos.x + (m_TargetPos.x - m_StartPos.x) * eased;
		m_Position.y = m_StartPos.y + (m_TargetPos.y - m_StartPos.y) * eased;
	}

	//重力変更入力
	if (!m_IsGravityMoving)
	{
		Gamepad_ThumbStick rs = Gamepad_GetRightStick(pad);
		if (rs.y > 0.5f || Keyboard_IsKeyDownTrigger(KK_UP))
			ChangeGravity(FACE_CEILING);
		else if (rs.y < -0.5f || Keyboard_IsKeyDownTrigger(KK_DOWN))
			ChangeGravity(FACE_FLOOR);
		else if (rs.x < -0.5f || Keyboard_IsKeyDownTrigger(KK_LEFT))
			ChangeGravity(FACE_LEFT_WALL);
		else if (rs.x > 0.5f || Keyboard_IsKeyDownTrigger(KK_RIGHT))
			ChangeGravity(FACE_RIGHT_WALL);
	}

	//重力移動補間
	if (m_IsGravityMoving)
	{
		m_GravityTimer += dt;
		float t = m_GravityTimer / m_GravityDuration;
		if (t >= 1.0f)
		{
			t = 1.0f;
			m_GravityFace = m_TargetFace;
			m_IsGravityMoving = false;
		}
		float eased = 1.0f - (1.0f - t) * (1.0f - t);
		m_Position.x = m_GravityStartPos.x + (m_TargetPos.x - m_GravityStartPos.x) * eased;
		m_Position.y = m_GravityStartPos.y + (m_TargetPos.y - m_GravityStartPos.y) * eased;
		m_Rotation.z = m_GravityStartRot.z + (m_TargetRot.z - m_GravityStartRot.z) * eased;
	}

	//ノーツヒット入力
	if (Keyboard_IsKeyDownTrigger(KK_SPACE))
	{
		JUDGE result = m_pNoteManager->Judge(m_LaneIndex, m_GravityFace);
		// TODO: result に基づいてスコア・コンボ処理
	}

}

void Player::Draw()
{
	if (m_Model) {
		ModelDraw(
			m_Model,
			m_Position,
			m_Rotation,
			m_Scale,
			m_Color,
			false,
			S_UNLIT
		);
	}
}

void Player::Finalize()
{}

void Player::MoveLeft()
{
	if (m_IsMoving || m_IsGravityMoving) return;
	int newLane = Clamp(m_LaneIndex - 1, (int)LANE_LEFT, (int)LANE_RIGHT);
	if (newLane == m_LaneIndex) return;
	m_TargetLaneIndex = newLane;
	m_StartPos = m_Position;
	m_TargetPos = CalcLaneTargetPos(m_TargetLaneIndex);
	m_MoveTimer = 0.0f;
	m_IsMoving = true;
}

void Player::MoveRight()
{
	if (m_IsMoving || m_IsGravityMoving) return;
	int newLane = Clamp(m_LaneIndex + 1, (int)LANE_LEFT, (int)LANE_RIGHT);
	if (newLane == m_LaneIndex) return;
	m_TargetLaneIndex = newLane;
	m_StartPos = m_Position;
	m_TargetPos = CalcLaneTargetPos(m_TargetLaneIndex);
	m_MoveTimer = 0.0f;
	m_IsMoving = true;
}

void Player::ChangeGravity(int targetFace)
{
	if (targetFace == m_GravityFace) return;

	m_TargetFace = targetFace;

	// 反対面ならレーンをそのまま、隣接面なら現在位置から最寄りレーンを計算
	bool isOpposite = (m_GravityFace + 2) % 4 == targetFace;
	m_LaneIndex = isOpposite ? m_LaneIndex : CalcNearestLane();
	m_TargetLaneIndex = m_LaneIndex;

	m_GravityStartPos = m_Position;
	m_GravityStartRot = m_Rotation;
	m_TargetPos = CalcFaceTargetPos(targetFace, m_LaneIndex);
	m_TargetRot = CalcFaceTargetRot(targetFace);

	// 回転を最短経路で補間するため差分を[-180, 180]に正規化
	float diff = m_TargetRot.z - m_GravityStartRot.z;
	while (diff > 180.0f)  diff -= 360.0f;
	while (diff < -180.0f) diff += 360.0f;
	m_TargetRot.z = m_GravityStartRot.z + diff;

	m_GravityTimer = 0.0f;
	m_IsGravityMoving = true;
	m_IsMoving = false; // レーン移動はキャンセル
}

int Player::CalcNearestLane()
{
	/*float axisVal = (m_GravityFace == FACE_FLOOR || m_GravityFace == FACE_CEILING)
		? m_Position.x : m_Position.y;
	int nearest = (int)roundf(axisVal / LANE_WIDTH);
	return Clamp(nearest, (int)LANE_LEFT, (int)LANE_RIGHT);*/

	switch (m_GravityFace)
	{
	case FACE::FACE_FLOOR:
	case FACE::FACE_LEFT_WALL:
		return LANE::LANE_LEFT;

	case FACE::FACE_CEILING:
	case FACE::FACE_RIGHT_WALL:
		return LANE::LANE_RIGHT;

	default:
		return LANE::LANE_CENTER;
	}
}

XMFLOAT3 Player::CalcFaceTargetPos(int face, int laneIndex)
{
	const float TUNNEL_HALF = 2.5f;
	XMFLOAT3 pos = m_Position;
	float laneVal = laneIndex * LANE_WIDTH;
	switch (face)
	{
	case FACE_FLOOR:      pos.x = laneVal;       pos.y = -TUNNEL_HALF; break;
	case FACE_CEILING:    pos.x = laneVal;       pos.y =  TUNNEL_HALF; break;
	case FACE_LEFT_WALL:  pos.x = -TUNNEL_HALF;  pos.y = laneVal;      break;
	case FACE_RIGHT_WALL: pos.x =  TUNNEL_HALF;  pos.y = laneVal;      break;
	}
	return pos;
}

XMFLOAT3 Player::CalcFaceTargetRot(int face)
{
	XMFLOAT3 rot = m_Rotation;
	switch (face)
	{
	case FACE_FLOOR:      rot.z =   0.0f; break;
	case FACE_LEFT_WALL:  rot.z =  90.0f; break;
	case FACE_CEILING:    rot.z = 180.0f; break;
	case FACE_RIGHT_WALL: rot.z = -90.0f; break;
	}
	return rot;
}

XMFLOAT3 Player::CalcLaneTargetPos(int laneIndex)
{
	XMFLOAT3 pos = m_Position;
	switch (m_GravityFace)
	{
	case FACE_FLOOR:
	case FACE_CEILING:
		pos.x = laneIndex * LANE_WIDTH;
		break;
	case FACE_LEFT_WALL:
	case FACE_RIGHT_WALL:
		pos.y = laneIndex * LANE_WIDTH;
		break;
	}
	return pos;
}
