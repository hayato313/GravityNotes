#pragma once

#include <d3d11.h>
#include "renderer.h"
#include "texture.h"
#include "component.h"
#include "model.h"
#include "glb_model.h"
#include "debug_ostream.h"
#include <DirectXMath.h>
using namespace DirectX;

class Sprite3D : public Transform3D
{
protected:
	MODEL* m_Model;
	GlbModel* m_GlbModel;     // GLB用モデル (.glb 拡張子の場合に使用)
	bool m_IsGlb;              // GLBモデルかどうか
	XMFLOAT3 m_ModelSize;
	XMFLOAT4 m_Color;
	XMFLOAT4 m_OriginalColor;
	bool m_UseOriginalColor;
	SHADERTYPE m_ShaderType;
public:
	Sprite3D(const XMFLOAT3& pos, const XMFLOAT3& scale, const XMFLOAT3& rot, const char* pass, SHADERTYPE st)
		: Transform3D(pos, rot, scale), m_Model(nullptr), m_GlbModel(nullptr), m_IsGlb(false),
		  m_Color(1.0f, 1.0f, 1.0f, 1.0f),
		  m_OriginalColor(1.0f, 1.0f, 1.0f, 1.0f), m_UseOriginalColor(true), m_ShaderType(st)
	{
		// 拡張子で読み込みを分岐
		if (IsGlbFile(pass))
		{
			m_IsGlb = true;
			m_GlbModel = new GlbModel();
			m_GlbModel->Load(pass, GetDevice(), GetDeviceContext());
			// GlbModel::Load() 内で aiProcess_GlobalScale (100倍) 適用済みなのでそのまま使用
			m_ModelSize = m_GlbModel->GetSize();
			m_OriginalColor = m_GlbModel->GetAverageMaterialColor();
		}
		else
		{
			m_IsGlb = false;
			m_Model = ModelLoad(pass);
			m_ModelSize = ModelGetSize(m_Model);
			m_OriginalColor = ModelGetAverageMaterialColor(m_Model);
		}
	}
	~Sprite3D()
	{
		if (m_IsGlb)
		{
			if (m_GlbModel)
			{
				m_GlbModel->Release();
				delete m_GlbModel;
				m_GlbModel = nullptr;
			}
		}
		else
		{
			ModelRelease(m_Model);
		}
	}

	virtual void Draw(void)
	{
		// 使用する色を決定
		XMFLOAT4 drawColor = m_UseOriginalColor ? m_OriginalColor : m_Color;
		bool shouldApplyColorReplace = !m_UseOriginalColor;

		if (!shouldApplyColorReplace)
		{
			drawColor.w = 0.0f;
		}

		if (m_IsGlb)
		{
			if (m_GlbModel && m_GlbModel->IsLoaded())
			{
				// GlbModel::Load() 内で aiProcess_GlobalScale (100倍) 適用済みなので追加スケール不要
				m_GlbModel->Draw(
					GetPos(),
					GetRot(),
					GetScale(),
					drawColor,
					shouldApplyColorReplace,
					m_ShaderType
				);
			}
			else
			{
			}
		}
		else
		{
			if (m_Model)
			{
				ModelDraw(
					m_Model,
					GetPos(),
					GetRot(),
					GetScale(),
					drawColor,
					shouldApplyColorReplace,
					m_ShaderType
				);
			}
			else
			{
			}
		}
	}

	// 色を設定
	void SetColor(const XMFLOAT4& color)
	{
		m_Color = color;
		m_UseOriginalColor = false;  // カスタム色を使用中
	}

	// 色を設定（R, G, B, A）
	void SetColor(float r, float g, float b, float a = 1.0f)
	{
		m_Color = XMFLOAT4(r, g, b, a);
		m_UseOriginalColor = false;  // カスタム色を使用中
	}

	// 色を取得
	XMFLOAT4 GetColor(void) const
	{
		return m_Color;
	}

	// 元の色を設定（初期化時に呼び出す）
	void SetOriginalColor(const XMFLOAT4& color)
	{
		m_OriginalColor = color;
	}

	// 元の色を設定（R, G, B, A）
	void SetOriginalColor(float r, float g, float b, float a = 1.0f)
	{
		m_OriginalColor = XMFLOAT4(r, g, b, a);
	}

	// 色をリセット（元の色に戻す）
	void ResetColor(void)
	{
		m_UseOriginalColor = true;
	}

	// 色を変更するメソッド（R, G, B, A 個別指定）
	void SetColorRed(float r) { m_Color.x = r; m_UseOriginalColor = false; }
	void SetColorGreen(float g) { m_Color.y = g; m_UseOriginalColor = false; }
	void SetColorBlue(float b) { m_Color.z = b; m_UseOriginalColor = false; }
	void SetColorAlpha(float a) { m_Color.w = a; m_UseOriginalColor = false; }

	// 色を取得するメソッド（R, G, B, A 個別取得）
	float GetColorRed(void) const { return m_Color.x; }
	float GetColorGreen(void) const { return m_Color.y; }
	float GetColorBlue(void) const { return m_Color.z; }
	float GetColorAlpha(void) const { return m_Color.w; }

	XMFLOAT3 GetModelSize(void) const { return m_ModelSize; }
	XMFLOAT3 GetDisplaySize(void) const 
	{ 
		XMFLOAT3 scale = GetScale();
		return XMFLOAT3(
			m_ModelSize.x * scale.x,
			m_ModelSize.y * scale.y,
			m_ModelSize.z * scale.z
		);
	}

	XMFLOAT4 GetModelColor(void) const
	{
		if (m_IsGlb)
		{
			return m_GlbModel ? m_GlbModel->GetAverageMaterialColor() : XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
		return ModelGetAverageMaterialColor(m_Model);
	}
};
