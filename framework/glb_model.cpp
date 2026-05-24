//==============================================================================
// GLBモデル読み込み・描画クラス [glb_model.cpp]
// Assimp + DirectX 11 による .glb (glTF Binary) 専用ローダー
//==============================================================================

#include "glb_model.h"
#include "renderer.h"
#include "texture.h"
#include "camera.h"
#include "debug_ostream.h"
#include "DirectXTex.h"
#include <windows.h>
#include <cstring>
#include <algorithm>
#include <float.h>

using namespace DirectX;

//==============================================================================
// 拡張子判定ユーティリティ
//==============================================================================
bool IsGlbFile(const char* filePath)
{
	if (!filePath) return false;
	size_t len = strlen(filePath);
	if (len < 4) return false;
	const char* ext = filePath + len - 4;
	return (_stricmp(ext, ".glb") == 0);
}

//==============================================================================
// コンストラクタ・デストラクタ
//==============================================================================
GlbModel::GlbModel()
{
}

GlbModel::~GlbModel()
{
	Release();
}

//==============================================================================
// モデル読み込み
//==============================================================================
bool GlbModel::Load(const char* filePath, ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	if (m_IsLoaded)
	{
		return false;
	}

	DWORD dwAttrib = GetFileAttributesA(filePath);
	if (dwAttrib == INVALID_FILE_ATTRIBUTES || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
	{
		return false;
	}


	// プロパティストアを作成してスケールを100倍に設定（GLBはcm単位、FBXはm単位のため）
	aiPropertyStore* props = aiCreatePropertyStore();
	aiSetImportPropertyFloat(props, AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, 100.0f);

	unsigned int pFlags =
		aiProcess_Triangulate |
		aiProcess_ConvertToLeftHanded |
		aiProcess_GenSmoothNormals |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GlobalScale;

	m_pScene = aiImportFileExWithProperties(filePath, pFlags, nullptr, props);

	aiReleasePropertyStore(props);

	if (!m_pScene || !m_pScene->mRootNode)
	{
		const char* err = aiGetErrorString();
		return false;
	}


	if (!LoadEmbeddedTextures(m_pScene, pDevice))
	{
	}

	m_pWhiteTexture = LoadTexture(L"asset\\texture\\fade.png");

	if (!ProcessMeshes(m_pScene, pDevice))
	{
		Release();
		return false;
	}

	SetupMeshMaterials(m_pScene);

	m_IsLoaded = true;
	return true;
}

//==============================================================================
// リソース解放
//==============================================================================
void GlbModel::Release()
{
	for (auto& mesh : m_Meshes)
	{
		SAFE_RELEASE(mesh.pVertexBuffer);
		SAFE_RELEASE(mesh.pIndexBuffer);
	}
	m_Meshes.clear();

	for (auto& pair : m_EmbeddedTextures)
	{
		SAFE_RELEASE(pair.second);
	}
	m_EmbeddedTextures.clear();

	m_pWhiteTexture = nullptr;

	if (m_pScene)
	{
		aiReleaseImport(m_pScene);
		m_pScene = nullptr;
	}

	m_IsLoaded = false;
}

//==============================================================================
// メッシュデータの抽出 + DX11バッファ生成
//==============================================================================
bool GlbModel::ProcessMeshes(const aiScene* pScene, ID3D11Device* pDevice)
{
	m_Meshes.resize(pScene->mNumMeshes);

	for (unsigned int m = 0; m < pScene->mNumMeshes; m++)
	{
		aiMesh* pMesh = pScene->mMeshes[m];
		GlbMesh& glbMesh = m_Meshes[m];

		std::vector<Vertex3D> vertices(pMesh->mNumVertices);

		for (unsigned int v = 0; v < pMesh->mNumVertices; v++)
		{
			vertices[v].position = XMFLOAT3(
				pMesh->mVertices[v].x,
				pMesh->mVertices[v].y,
				pMesh->mVertices[v].z
			);

			if (pMesh->HasNormals())
			{
				vertices[v].normal = XMFLOAT3(
					pMesh->mNormals[v].x,
					pMesh->mNormals[v].y,
					pMesh->mNormals[v].z
				);
			}
			else
			{
				vertices[v].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			}

			if (pMesh->HasTextureCoords(0))
			{
				vertices[v].texCoord = XMFLOAT2(
					pMesh->mTextureCoords[0][v].x,
					pMesh->mTextureCoords[0][v].y
				);
			}
			else
			{
				vertices[v].texCoord = XMFLOAT2(0.0f, 0.0f);
			}

			vertices[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		std::vector<unsigned int> indices;
		indices.reserve(pMesh->mNumFaces * 3);

		for (unsigned int f = 0; f < pMesh->mNumFaces; f++)
		{
			const aiFace& face = pMesh->mFaces[f];
			if (face.mNumIndices == 3)
			{
				indices.push_back(face.mIndices[0]);
				indices.push_back(face.mIndices[1]);
				indices.push_back(face.mIndices[2]);
			}
		}

		glbMesh.indexCount = (unsigned int)indices.size();

		if (pMesh->mNumVertices == 0 || glbMesh.indexCount == 0)
		{
			continue;
		}

		{
			D3D11_BUFFER_DESC bd = {};
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(Vertex3D) * pMesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			D3D11_SUBRESOURCE_DATA sd = {};
			sd.pSysMem = vertices.data();

			HRESULT hr = pDevice->CreateBuffer(&bd, &sd, &glbMesh.pVertexBuffer);
			if (FAILED(hr))
			{
				return false;
			}
		}

		{
			D3D11_BUFFER_DESC bd = {};
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * glbMesh.indexCount;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

			D3D11_SUBRESOURCE_DATA sd = {};
			sd.pSysMem = indices.data();

			HRESULT hr = pDevice->CreateBuffer(&bd, &sd, &glbMesh.pIndexBuffer);
			if (FAILED(hr))
			{
				return false;
			}
		}

	}

	return true;
}

//==============================================================================
// GLB埋め込みテクスチャの処理
//==============================================================================
bool GlbModel::LoadEmbeddedTextures(const aiScene* pScene, ID3D11Device* pDevice)
{
	bool allSucceeded = true;

	for (unsigned int i = 0; i < pScene->mNumTextures; i++)
	{
		const aiTexture* pAiTex = pScene->mTextures[i];
		if (!pAiTex) continue;

		ID3D11ShaderResourceView* pSRV = nullptr;
		TexMetadata metadata;
		ScratchImage image;

		if (pAiTex->mHeight == 0)
		{
			if (pAiTex->pcData && pAiTex->mWidth > 0)
			{
				HRESULT hr = LoadFromWICMemory(
					reinterpret_cast<const uint8_t*>(pAiTex->pcData),
					static_cast<size_t>(pAiTex->mWidth),
					WIC_FLAGS_NONE,
					&metadata,
					image
				);

				if (SUCCEEDED(hr))
				{
					hr = CreateShaderResourceView(
						pDevice,
						image.GetImages(),
						image.GetImageCount(),
						metadata,
						&pSRV
					);
				}

				if (FAILED(hr))
				{
					allSucceeded = false;
				}
			}
		}
		else
		{
			if (pAiTex->pcData && pAiTex->mWidth > 0 && pAiTex->mHeight > 0)
			{
				HRESULT hr = image.Initialize2D(
					DXGI_FORMAT_R8G8B8A8_UNORM,
					static_cast<size_t>(pAiTex->mWidth),
					static_cast<size_t>(pAiTex->mHeight),
					1, 1
				);

				if (SUCCEEDED(hr))
				{
					const Image* pImg = image.GetImage(0, 0, 0);
					if (pImg && pImg->pixels)
					{
						size_t byteSize = static_cast<size_t>(pAiTex->mWidth)
							* static_cast<size_t>(pAiTex->mHeight) * 4;
						memcpy(pImg->pixels,
							reinterpret_cast<const uint8_t*>(pAiTex->pcData),
							byteSize);

						metadata = image.GetMetadata();
						hr = CreateShaderResourceView(
							pDevice,
							image.GetImages(),
							image.GetImageCount(),
							metadata,
							&pSRV
						);
					}
				}

				if (FAILED(hr))
				{
					allSucceeded = false;
				}
			}
		}

		if (pSRV)
		{
			std::string texName = pAiTex->mFilename.length > 0
				? std::string(pAiTex->mFilename.data)
				: std::string("*") + std::to_string(i);

			m_EmbeddedTextures[texName] = pSRV;

		}
	}

	return allSucceeded;
}

//==============================================================================
// マテリアルとテクスチャの紐づけ
//==============================================================================
void GlbModel::SetupMeshMaterials(const aiScene* pScene)
{
	for (unsigned int m = 0; m < pScene->mNumMeshes; m++)
	{
		const aiMesh* pMesh = pScene->mMeshes[m];
		GlbMesh& glbMesh = m_Meshes[m];

		if (pMesh->mMaterialIndex < pScene->mNumMaterials)
		{
			const aiMaterial* pMat = pScene->mMaterials[pMesh->mMaterialIndex];

			aiColor4D diffuse(1.0f, 1.0f, 1.0f, 1.0f);
			if (AI_SUCCESS == pMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
			{
				glbMesh.diffuseColor = XMFLOAT4(diffuse.r, diffuse.g, diffuse.b, diffuse.a);
			}

			if (diffuse.r == 0.0f && diffuse.g == 0.0f && diffuse.b == 0.0f)
			{
				aiColor4D baseColor;
				if (AI_SUCCESS == pMat->Get(AI_MATKEY_BASE_COLOR, baseColor))
				{
					glbMesh.diffuseColor = XMFLOAT4(baseColor.r, baseColor.g, baseColor.b, baseColor.a);
				}
			}

			// アルファ値の補正
			if (glbMesh.diffuseColor.w == 0.0f)
			{
				glbMesh.diffuseColor.w = 1.0f;
			}

			aiString texPath;
			if (AI_SUCCESS == pMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath))
			{
				std::string key(texPath.data);

				if (m_EmbeddedTextures.count(key) > 0)
				{
					glbMesh.pTextureSRV = m_EmbeddedTextures[key];
				}
				else
				{
					const aiTexture* pEmbedded = pScene->GetEmbeddedTexture(texPath.data);
					if (pEmbedded)
					{
						std::string embName = pEmbedded->mFilename.length > 0
							? std::string(pEmbedded->mFilename.data) : key;

						if (m_EmbeddedTextures.count(embName) > 0)
						{
							glbMesh.pTextureSRV = m_EmbeddedTextures[embName];
						}
					}
				}
			}

			if (!glbMesh.pTextureSRV)
			{
				aiString baseTexPath;
				if (AI_SUCCESS == pMat->GetTexture(aiTextureType_BASE_COLOR, 0, &baseTexPath))
				{
					std::string key(baseTexPath.data);
					if (m_EmbeddedTextures.count(key) > 0)
					{
						glbMesh.pTextureSRV = m_EmbeddedTextures[key];
					}
					else
					{
						const aiTexture* pEmbedded = pScene->GetEmbeddedTexture(baseTexPath.data);
						if (pEmbedded)
						{
							std::string embName = pEmbedded->mFilename.length > 0
								? std::string(pEmbedded->mFilename.data) : key;
							if (m_EmbeddedTextures.count(embName) > 0)
							{
								glbMesh.pTextureSRV = m_EmbeddedTextures[embName];
							}
						}
					}
				}
			}

			if (!glbMesh.pTextureSRV)
			{
				glbMesh.pTextureSRV = m_pWhiteTexture;
			}
		}
	}
}

//==============================================================================
// モデルのバウンディングボックスサイズを取得
//==============================================================================
XMFLOAT3 GlbModel::GetSize() const
{
	if (!m_pScene) return XMFLOAT3(0.0f, 0.0f, 0.0f);

	XMFLOAT3 minB(FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 maxB(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	for (unsigned int m = 0; m < m_pScene->mNumMeshes; m++)
	{
		aiMesh* pMesh = m_pScene->mMeshes[m];
		for (unsigned int v = 0; v < pMesh->mNumVertices; v++)
		{
			aiVector3D pos = pMesh->mVertices[v];
			minB.x = (std::min)(minB.x, pos.x);
			minB.y = (std::min)(minB.y, pos.y);
			minB.z = (std::min)(minB.z, pos.z);
			maxB.x = (std::max)(maxB.x, pos.x);
			maxB.y = (std::max)(maxB.y, pos.y);
			maxB.z = (std::max)(maxB.z, pos.z);
		}
	}

	return XMFLOAT3(maxB.x - minB.x, maxB.y - minB.y, maxB.z - minB.z);
}

//==============================================================================
// 全マテリアルの平均色を取得
//==============================================================================
XMFLOAT4 GlbModel::GetAverageMaterialColor() const
{
	if (m_Meshes.empty()) return XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	float r = 0.0f, g = 0.0f, b = 0.0f, a = 0.0f;
	unsigned int count = (unsigned int)m_Meshes.size();

	for (unsigned int i = 0; i < count; i++)
	{
		r += m_Meshes[i].diffuseColor.x;
		g += m_Meshes[i].diffuseColor.y;
		b += m_Meshes[i].diffuseColor.z;
		a += m_Meshes[i].diffuseColor.w;
	}

	return XMFLOAT4(r / count, g / count, b / count, a / count);
}

//==============================================================================
// 描画処理 (pos/rot/scale指定版)
//==============================================================================
void GlbModel::Draw(XMFLOAT3 pos, XMFLOAT3 rot, XMFLOAT3 scale,
	const XMFLOAT4& color, bool useColorReplace, SHADERTYPE shadertype)
{
	if (!m_IsLoaded) return;

	Camera* pCamera = GetCamera();
	if (!pCamera) return;

	// 頂点レイアウトとシェーダーのセット
	GetDeviceContext()->IASetInputLayout(GetShader(shadertype)->GetVertexLayout());
	GetDeviceContext()->VSSetShader(GetShader(shadertype)->GetVertexShader(), NULL, 0);
	GetDeviceContext()->PSSetShader(GetShader(shadertype)->GetPixelShader(), NULL, 0);

	XMMATRIX View = pCamera->GetView();
	XMMATRIX Projection = pCamera->GetProjection();

	XMMATRIX TranslationMatrix = XMMatrixTranslation(pos.x, pos.y, pos.z);
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(rot.x),
		XMConvertToRadians(rot.y),
		XMConvertToRadians(rot.z));
	XMMATRIX ScalingMatrix = XMMatrixScaling(scale.x, scale.y, scale.z);

	XMMATRIX World = ScalingMatrix * RotationMatrix * TranslationMatrix;

	SetWorldMatrix(World);
	SetViewMatrix(View);
	SetProjectionMatrix(Projection);

	ID3D11DeviceContext* pContext = GetDeviceContext();
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (unsigned int m = 0; m < (unsigned int)m_Meshes.size(); m++)
	{
		const GlbMesh& mesh = m_Meshes[m];

		if (!mesh.pVertexBuffer || !mesh.pIndexBuffer || mesh.indexCount == 0)
			continue;

		XMFLOAT4 finalColor;
		if (useColorReplace)
		{
			finalColor = color;
		}
		else
		{
			if (color.w == 0.0f)
			{
				finalColor = mesh.diffuseColor;
			}
			else
			{
				finalColor = XMFLOAT4(
					mesh.diffuseColor.x * color.x,
					mesh.diffuseColor.y * color.y,
					mesh.diffuseColor.z * color.z,
					mesh.diffuseColor.w * color.w
				);
			}
		}
		(void)finalColor;

		ID3D11ShaderResourceView* pSRV = mesh.pTextureSRV;
		pContext->PSSetShaderResources(0, 1, &pSRV);

		UINT stride = sizeof(Vertex3D);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &mesh.pVertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh.pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);

		pContext->DrawIndexed(mesh.indexCount, 0, 0);
	}
}
