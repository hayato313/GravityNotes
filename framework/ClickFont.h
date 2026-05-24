/*==============================================================================

   クリック可能フォント [ClickFont.h]

==============================================================================*/
#pragma once

#include <DirectXMath.h>
#include <functional>
#include <string>

#include "font.h"
#include "mouse.h"

using namespace DirectX;

class ClickFont : public FontRenderer
{
public:
	using ClickHandler = std::function<void()>;

	ClickFont(XMFLOAT2 pos, float fontSize, float rotation,
		XMFLOAT4 normalColor, XMFLOAT4 hoverColor, const std::string& text);
	~ClickFont() = default;

	void Update();
	void SetOnClick(ClickHandler handler) { m_OnClick = std::move(handler); }

	void SetHitSize(XMFLOAT2 size) { m_HitSize = size; }
	XMFLOAT2 GetHitSize() const { return m_HitSize; }

	bool IsHover() const { return m_IsHover; }

private:
	bool HitTest(int mouseX, int mouseY) const;

	XMFLOAT4 m_NormalColor;
	XMFLOAT4 m_HoverColor;
	XMFLOAT2 m_HitSize;
	bool m_IsHover;
	bool m_WasLeftDown;
	ClickHandler m_OnClick;
};
