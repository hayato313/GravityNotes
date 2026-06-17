#include "texture.h"
#include <Windows.h>
#include <cstdio>
#include <vector>
#include <cstdint>
#include <string>

namespace
{
ID3D11ShaderResourceView* LoadTextureWithFlags(const wchar_t* texpass, WIC_FLAGS flags)
{
	TexMetadata metadata;
	ScratchImage image;
	ID3D11ShaderResourceView* g_Texture = nullptr;

	// flagsで「色テクスチャとして読むか」「データテクスチャとして読むか」を切り替える。
	HRESULT hr = LoadFromWICFile(texpass, flags, &metadata, image);
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
}

ID3D11ShaderResourceView* LoadTexture(const wchar_t* texpass)
{
	// 通常の絵のテクスチャは色なので、sRGBとして読み込む。
	return LoadTextureWithFlags(texpass, WIC_FLAGS_FORCE_SRGB);
}

ID3D11ShaderResourceView* LoadTexture(const std::wstring& texpass)
{
	return LoadTexture(texpass.c_str());
}

ID3D11ShaderResourceView* LoadTextureLinear(const wchar_t* texpass)
{
	// NormalMapはRGBを法線データとして使うため、sRGB補正をかけずにそのまま読む。
	return LoadTextureWithFlags(texpass, WIC_FLAGS_IGNORE_SRGB);
}

ID3D11ShaderResourceView* LoadTextureLinear(const std::wstring& texpass)
{
	return LoadTextureLinear(texpass.c_str());
}
