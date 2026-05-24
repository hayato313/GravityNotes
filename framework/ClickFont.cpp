/*==============================================================================

   クリック可能フォント [ClickFont.cpp]

==============================================================================*/
#include "ClickFont.h"
#include "renderer.h"
#include "shadermanager.h"
#include "define.h"

ClickFont::ClickFont(XMFLOAT2 pos, float fontSize, float rotation,
	XMFLOAT4 normalColor, XMFLOAT4 hoverColor, const std::string& text)
	: FontRenderer(pos, fontSize, rotation, normalColor, text)
	, m_NormalColor(normalColor)
	, m_HoverColor(hoverColor)
	, m_HitSize({ 200.0f, fontSize })
	, m_IsHover(false)
	, m_WasLeftDown(false)
	, m_OnClick(nullptr)
{
}

bool ClickFont::HitTest(int mouseX, int mouseY) const
{
	// ウィンドウサイズが変わっても正しく判定するため、
	// マウス座標（クライアントピクセル）を論理座標（SCREEN_WIDTH/HEIGHT基準）に変換する
	const float clientW = Direct3D_GetClientWidth();
	const float clientH = Direct3D_GetClientHeight();
	const float targetAspect = SCREEN_WIDTH / SCREEN_HEIGHT;
	const float windowAspect = clientW / clientH;

	// Direct3D_SetViewport2D() と同じ黒帯計算
	float vpX, vpY, vpW, vpH;
	if (windowAspect > targetAspect)
	{
		vpH = clientH;
		vpW = clientH * targetAspect;
		vpX = (clientW - vpW) * 0.5f;
		vpY = 0.0f;
	}
	else
	{
		vpW = clientW;
		vpH = clientW / targetAspect;
		vpX = 0.0f;
		vpY = (clientH - vpH) * 0.5f;
	}

	// ビューポート内の相対位置 → 論理座標へ
	const float logicalX = (mouseX - vpX) / vpW * SCREEN_WIDTH;
	const float logicalY = (mouseY - vpY) / vpH * SCREEN_HEIGHT;

	const XMFLOAT2 pos = GetPos();
	const float halfW = m_HitSize.x * 0.5f;
	const float halfH = m_HitSize.y * 0.5f;

	return (logicalX >= pos.x - halfW && logicalX <= pos.x + halfW
		&& logicalY >= pos.y - halfH && logicalY <= pos.y + halfH);
}

void ClickFont::Update()
{
	Mouse_State ms{};
	Mouse_GetState(&ms);

	const bool hover = HitTest(ms.x, ms.y);
	if (hover != m_IsHover) {
		m_IsHover = hover;
		SetColor(m_IsHover ? m_HoverColor : m_NormalColor);
	}

	const bool leftDown = ms.leftButton;
	const bool pressedThisFrame = (leftDown && !m_WasLeftDown);
	m_WasLeftDown = leftDown;

	if (pressedThisFrame && m_IsHover) {
		if (m_OnClick) {
			m_OnClick();
		}
	}
}
