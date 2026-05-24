#include "texture.h"
#include <Windows.h>
#include <cstdio>
#include <vector>
#include <cstdint>
#include <string>

ID3D11ShaderResourceView* LoadTexture(const wchar_t* texpass)
{
	TexMetadata metadata;
	ScratchImage image;
	ID3D11ShaderResourceView* g_Texture = nullptr;

	// 標準的な方法でロード（戻り値をチェック）
	HRESULT hr = LoadFromWICFile(texpass, WIC_FLAGS_FORCE_SRGB, &metadata, image);
	if (FAILED(hr))
	{
		return nullptr;
	}

	// 標準的に SRV を作成（戻り値をチェック）
	hr = CreateShaderResourceView(
		GetDevice(),
		image.GetImages(),
		image.GetImageCount(),
		metadata,
		&g_Texture
	);

	if (FAILED(hr) || g_Texture == nullptr)
	{
		// 失敗時は NULL を返す（呼び出し側でフォールバック処理を行う）
		return nullptr;
	}

	std::string texpassStr;
	size_t len = wcslen(texpass);
	for (size_t i = 0; i < len; ++i) texpassStr += static_cast<char>(texpass[i]);
	hal::dout << texpassStr << std::endl;
	
	return g_Texture;
}

ID3D11ShaderResourceView* LoadTexture(const std::wstring& texpass)
{
	return LoadTexture(texpass.c_str());
}