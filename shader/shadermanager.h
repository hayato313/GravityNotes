#pragma once

#include "main.h"
#include <string>
#include "renderer.h"
using namespace DirectX;

enum SHADERTYPE {
	S_UNLIT = 0,
	S_LAMBERT,
	S_PHONG,
	S_RIM_LIGHT,
	S_OUTLINE,
	S_SHADOW_MAP,		// ShadowMap作成用。色ではなく深度だけを描く。	
	S_SHADOW_RECEIVE,	// ShadowMapを読んで、床などに影を反映する。
	S_MAX,
};

const std::string filenames[S_MAX] = {
	"UnlitTexture",
	"VertexDirectionalLighting",
	"PixelDirectionalLighting",
	"RimLight",
	"Outline",
	"ShadowMap",
	"ShadowReceive"
};

class ShaderManager
{
private:
	ID3D11InputLayout* m_Layout = nullptr;
	ID3D11VertexShader* m_VS = nullptr;
	ID3D11PixelShader* m_PS = nullptr;

public:
	ShaderManager() = delete;
	ShaderManager(SHADERTYPE st)
	{
		std::string vsname = "shader/" + filenames[st] + "VS.cso";
		std::string psname = "shader/" + filenames[st] + "PS.cso";

		// シェーダー読み込み
		CreateVertexShader(&m_VS, &m_Layout, vsname.c_str());
		CreatePixelShader(&m_PS, psname.c_str());
	}
	~ShaderManager()
	{
		SAFE_RELEASE(m_Layout);
		SAFE_RELEASE(m_VS);
		SAFE_RELEASE(m_PS);
	}
	ID3D11InputLayout* GetVertexLayout() { return m_Layout; }
	ID3D11VertexShader* GetVertexShader() { return m_VS; }
	ID3D11PixelShader* GetPixelShader() { return m_PS; }
};

inline ShaderManager*& ShaderSlot(SHADERTYPE st)
{
	static ShaderManager* SMng[S_MAX] = {};
	return SMng[st];
}

inline void InitShader(void)
{
	for (int i = 0; i < S_MAX; i++)
	{
		ShaderSlot((SHADERTYPE)i) = new ShaderManager((SHADERTYPE)i);
	}
}

inline void FinalizeShader(void)
{
	for (int i = 0; i < S_MAX; i++)
	{
		delete ShaderSlot((SHADERTYPE)i);
		ShaderSlot((SHADERTYPE)i) = nullptr;
	}
}

inline ShaderManager* GetShader(SHADERTYPE st)
{
	return ShaderSlot(st);
}

