#pragma once

#include "main.h"
#include <string>
#include "renderer.h"
using namespace DirectX;

enum SHADERTYPE {
	S_UNLIT = 0,
	S_LAMBERT,
	S_PHONG,
	S_MAX,
};

const std::string filenames[S_MAX] = {
	"UnlitTexture",
	"VertexDirectionalLighting",
	"PixelDirectionalLighting"
};

class ShaderManager
{
private:
	ID3D11InputLayout* m_Layout;
	ID3D11VertexShader* m_VS;
	ID3D11PixelShader* m_PS;

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
	~ShaderManager() = default;
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

