#pragma once
//==============================================================================
// GLBモデル読み込み・描画クラス [glb_model.h]
// Assimp + DirectX 11 による .glb (glTF Binary) 専用ローダー
//==============================================================================

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <unordered_map>

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#pragma comment(lib, "assimp-vc143-mt.lib")

#include "model.h"
#include "renderer.h"

using namespace DirectX;

//==============================================================================
// メッシュ単位のデータ
//==============================================================================
struct GlbMesh
{
	ID3D11Buffer* pVertexBuffer = nullptr;
	ID3D11Buffer* pIndexBuffer = nullptr;
	unsigned int  indexCount = 0;

	// マテリアル情報
	XMFLOAT4 diffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
	ID3D11ShaderResourceView* pTextureSRV = nullptr;  // テクスチャ (nullなら白テクスチャを使用)
};

//==============================================================================
// GLBモデルクラス
//==============================================================================
class GlbModel
{
public:
	GlbModel();
	~GlbModel();

	// モデル読み込み (.glb ファイルパス)
	bool Load(const char* filePath, ID3D11Device* pDevice, ID3D11DeviceContext* pContext);

	// リソース解放
	void Release();

	// 描画 (pos/rot/scale指定版 — Sprite3Dと同じインターフェース)
	void Draw(XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT3 scale,
		const XMFLOAT4& color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		bool useColorReplace = false,SHADERTYPE shadertype = S_LAMBERT);

	// メッシュ数を取得
	unsigned int GetMeshCount() const { return (unsigned int)m_Meshes.size(); }

	// 読み込み済みかどうか
	bool IsLoaded() const { return m_IsLoaded; }

	// モデルのバウンディングボックスサイズを取得
	XMFLOAT3 GetSize() const;

	// 全マテリアルの平均色を取得
	XMFLOAT4 GetAverageMaterialColor() const;

private:
	// Assimpシーンからメッシュデータを抽出しDX11バッファを作成
	bool ProcessMeshes(const aiScene* pScene, ID3D11Device* pDevice);

	// GLB埋め込みテクスチャの読み込み
	bool LoadEmbeddedTextures(const aiScene* pScene, ID3D11Device* pDevice);

	// マテリアルからテクスチャパスを取得し、埋め込みテクスチャとマッピング
	void SetupMeshMaterials(const aiScene* pScene);

private:
	bool m_IsLoaded = false;
	const aiScene* m_pScene = nullptr;

	std::vector<GlbMesh> m_Meshes;

	// 埋め込みテクスチャのキャッシュ (テクスチャ名 → SRV)
	std::unordered_map<std::string, ID3D11ShaderResourceView*> m_EmbeddedTextures;

	// テクスチャなしメッシュ用の白テクスチャ
	ID3D11ShaderResourceView* m_pWhiteTexture = nullptr;
};

// 拡張子が .glb かどうかを判定するユーティリティ関数
bool IsGlbFile(const char* filePath);
