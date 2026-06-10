/*==============================================================================

   レンダリング管理[renderer.h]
														 Author :
														 Date   :
--------------------------------------------------------------------------------

==============================================================================*/
#pragma once

#include "main.h"

//*********************************************************
// 構造体
//*********************************************************

struct Vertex3D
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT4 color;
	XMFLOAT2 texCoord;
};

struct VertexSkinned
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT4 color;
	XMFLOAT2 texCoord;
	UINT boneIndex[4];
	float boneWeight[4];
};

#define SAFE_RELEASE(p) do { if (p) { (p)->Release(); (p) = nullptr; } } while (0)

// 頂点構造体
struct VERTEX_3D
{
	XMFLOAT3 Position;	//頂点座標　XMFLOAT3 ＝　float x,y,z
	XMFLOAT3 Normal;	//法線ベクトル 
	XMFLOAT4 Diffuse;	//色  XMFLOAT4 = float x,y,z,w
	XMFLOAT2 TexCoord;	//テクスチャ座標 XMFLOAT2 = float x,y
};

// マテリアル構造体
struct MATERIAL
{
	XMFLOAT4	Ambient;	//アンビエント
	XMFLOAT4	Diffuse;	//デフューズ
	XMFLOAT4	Specular;	//スペキュラ
	XMFLOAT4	Emission;	//エミッシブ
	float		Shininess;	//スペキュラパラメータ
	float		Dummy[3];	//16byte境界調整用パディング
};

struct LIGHT
{
	BOOL		Enable;
	BOOL		Dummy[3];//16byte境界用
	XMFLOAT4	Direction;
	XMFLOAT4	Diffuse;
	XMFLOAT4	Ambient;

	XMFLOAT4	Position;
	XMFLOAT4	PointLightParam;
};

// シャドウマップを読むためにシェーダーへ渡す定数。
// LightViewProjection は「ライトから見た世界」に変換する行列。
struct SHADOW_CONSTANT
{
	XMFLOAT4X4 LightViewProjection;
	XMFLOAT4 Param;	// x:深度のずれ防止 y:影部分の明るさ z,w:ShadowMapの1テクセル分のUVサイズ
};

enum	BLENDSTATE
{
	BLENDSTATE_NONE = 0,	//ブレンドしない
	BLENDSTATE_ALFA,		//普通のαブレンド
	BLENDSTATE_ADD,			//加算合成 
	BLENDSTATE_SUB,			//減算合成

	BLENDSTATE_MAX
};

enum CULLSTATE
{
	CULLSTATE_NONE = 0,
	CULLSTATE_FRONT,
	CULLSTATE_BACK,

	CULLSTATE_MAX
};

void SetBlendState(BLENDSTATE blend);
void SetCullState(CULLSTATE cull);

//*****************************************************************************
// プロトタイプ宣言
//*****************************************************************************
HRESULT InitRenderer(HINSTANCE hInstance, HWND hWnd, BOOL bWindow);
void FinalizeRenderer(void);

void Clear(void);
void Present(void);

ID3D11Device *GetDevice( void );
ID3D11DeviceContext *GetDeviceContext( void );

void SetDepthEnable( bool Enable );

void SetWorldViewProjection2D(void);
void ResetWorldViewProjection3D(void);


void SetWorldMatrix(XMMATRIX WorldMatrix );
void SetViewMatrix(XMMATRIX ViewMatrix );
void SetProjectionMatrix(XMMATRIX ProjectionMatrix );

void SetCameraPosition(XMFLOAT3 CameraPosition);

void SetParameter(XMFLOAT4 Parameter);

// ShadowMap用の行列と調整値をGPUへ送る。
void SetShadowMatrix(XMMATRIX LightViewProjection, XMFLOAT4 Param);

// ここから先の描画を、画面ではなくShadowMapへ書き込む。
void BeginShadowMap(void);

// ShadowMapへの描画を終えて、通常の画面描画へ戻す。
void EndShadowMap(void);


void SetMaterial( MATERIAL Material );

void CreateVertexShader(ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName);
void CreatePixelShader(ID3D11PixelShader** PixelShader, const char* FileName);

void SetLight(LIGHT Light);

void Direct3D_ResizeWindow(unsigned int clientW, unsigned int clientH);
float Direct3D_GetClientWidth(void);
float Direct3D_GetClientHeight(void);
void Direct3D_Resize(unsigned int width, unsigned int height);

