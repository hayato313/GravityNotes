/*==============================================================================

   レンダリング管理 [renderer.cpp]
														 Author :
														 Date   :
--------------------------------------------------------------------------------

==============================================================================*/
#include <io.h>
#include "renderer.h"
#include "define.h"



//*********************************************************
// 構造体
//*********************************************************


//*****************************************************************************
// グローバル変数:
//*****************************************************************************
D3D_FEATURE_LEVEL       g_FeatureLevel = D3D_FEATURE_LEVEL_11_0;

ID3D11Device*           g_D3DDevice = NULL;
ID3D11DeviceContext*    g_ImmediateContext = NULL;
IDXGISwapChain*         g_SwapChain = NULL;
ID3D11RenderTargetView* g_RenderTargetView = NULL;
ID3D11DepthStencilView* g_DepthStencilView = NULL;



ID3D11VertexShader*     g_VertexShader = NULL;
ID3D11PixelShader*      g_PixelShader = NULL;
ID3D11InputLayout*      g_VertexLayout = NULL;

ID3D11Buffer*			g_WorldViewProjection = NULL;

ID3D11Buffer*			g_WorldBuffer = NULL;
ID3D11Buffer*			g_ViewBuffer = NULL;
ID3D11Buffer*			g_ProjectionBuffer = NULL;
ID3D11Buffer*			g_MaterialBuffer = NULL;
ID3D11Buffer*			g_LightBuffer = NULL;
ID3D11Buffer*			g_CameraBuffer = NULL;
ID3D11Buffer*			g_ParameterBuffer = NULL;
ID3D11Buffer*			g_ShadowBuffer = NULL;




XMMATRIX				g_WorldMatrix;
XMMATRIX				g_ViewMatrix;
XMMATRIX				g_ProjectionMatrix;

ID3D11DepthStencilState* g_DepthStateEnable;
ID3D11DepthStencilState* g_DepthStateDisable;

static float	bFactor[4] = { 0.0f,0.0f,0.0f,0.0f };
static ID3D11BlendState* bState[BLENDSTATE_MAX];
static ID3D11RasterizerState* rState[CULLSTATE_MAX];

// ShadowMapは、ライトから見た「深度だけの画像」。
// Texture本体、深度書き込み用View、シェーダーで読む用View、読み取り用Samplerを分けて持つ。
static const UINT SHADOW_MAP_SIZE = 2048;
static ID3D11Texture2D* g_ShadowMapTexture = NULL;
static ID3D11DepthStencilView* g_ShadowMapDepthView = NULL;
static ID3D11ShaderResourceView* g_ShadowMapShaderView = NULL;
static ID3D11SamplerState* g_ShadowMapSampler = NULL;

// ウィンドウクライアントサイズ（ビューポート計算用）
static float g_ClientWidth  = DRAW_SCREEN_WIDTH;
static float g_ClientHeight = DRAW_SCREEN_HEIGHT;

// バックバッファ情報（リサイズ時に参照）
static D3D11_TEXTURE2D_DESC g_BackBufferDesc;

// 深度ステンシルバッファ（解放用に保持）
static ID3D11Texture2D* g_pDepthStencilBuffer = NULL;

// =====================================================
// 内部ヘルパー：バックバッファ/深度バッファ解放
// =====================================================
static void releaseBackBuffer(void)
{
	if (g_ImmediateContext)
		g_ImmediateContext->OMSetRenderTargets(0, NULL, NULL);
	SAFE_RELEASE(g_RenderTargetView);
	SAFE_RELEASE(g_pDepthStencilBuffer);
	SAFE_RELEASE(g_DepthStencilView);
}

// =====================================================
// 内部ヘルパー：バックバッファ/深度バッファ生成
// =====================================================
static void configureBackBuffer(void)
{
	// バックバッファから RenderTargetView 生成
	ID3D11Texture2D* pBackBuffer = NULL;
	g_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	g_D3DDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_RenderTargetView);
	pBackBuffer->GetDesc(&g_BackBufferDesc);
	pBackBuffer->Release();

	// 深度ステンシルバッファ生成
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width              = g_BackBufferDesc.Width;
	td.Height             = g_BackBufferDesc.Height;
	td.MipLevels          = 1;
	td.ArraySize          = 1;
	td.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
	td.SampleDesc.Count   = 1;
	td.SampleDesc.Quality = 0;
	td.Usage              = D3D11_USAGE_DEFAULT;
	td.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
	td.CPUAccessFlags     = 0;
	td.MiscFlags          = 0;
	g_D3DDevice->CreateTexture2D(&td, NULL, &g_pDepthStencilBuffer);

	// 深度ステンシルビュー生成
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
	ZeroMemory(&dsvd, sizeof(dsvd));
	dsvd.Format        = td.Format;
	dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvd.Flags         = 0;
	g_D3DDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &dsvd, &g_DepthStencilView);

	// DirectX へセット
	g_ImmediateContext->OMSetRenderTargets(1, &g_RenderTargetView, g_DepthStencilView);

	// ビューポートをバックバッファ全体に設定
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width    = (FLOAT)g_BackBufferDesc.Width;
	vp.Height   = (FLOAT)g_BackBufferDesc.Height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	g_ImmediateContext->RSSetViewports(1, &vp);
}

// =====================================================
// 内部ヘルパー：2D ビューポート設定（spec §2.1）
// アスペクト比を保ちレターボックス/ピラーボックスを生成
// =====================================================
static void Direct3D_SetViewport2D(void)
{
	const float targetAspect = DRAW_SCREEN_WIDTH / DRAW_SCREEN_HEIGHT;
	const float windowAspect = g_ClientWidth / g_ClientHeight;

	float vpW, vpH, vpX = 0.0f, vpY = 0.0f;

	if (windowAspect > targetAspect)
	{
		// 横長: 縦を基準、左右に余白
		vpH = g_ClientHeight;
		vpW = g_ClientHeight * targetAspect;
		vpX = (g_ClientWidth - vpW) * 0.5f;
	}
	else
	{
		// 縦長または同等: 横を基準、上下に余白
		vpW = g_ClientWidth;
		vpH = g_ClientWidth / targetAspect;
		vpY = (g_ClientHeight - vpH) * 0.5f;
	}

	// クライアントサイズとバックバッファサイズの比率差を補正
	const float scaleX = (float)g_BackBufferDesc.Width  / g_ClientWidth;
	const float scaleY = (float)g_BackBufferDesc.Height / g_ClientHeight;

	D3D11_VIEWPORT vp;
	vp.TopLeftX = vpX * scaleX;
	vp.TopLeftY = vpY * scaleY;
	vp.Width    = vpW * scaleX;
	vp.Height   = vpH * scaleY;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	g_ImmediateContext->RSSetViewports(1, &vp);
}

// =====================================================
// 内部ヘルパー：3D ビューポート設定（spec §2.2）
// 2D と逆方向にはみ出す
// =====================================================
static void Direct3D_SetViewport3D(void)
{
	const float targetAspect = DRAW_SCREEN_WIDTH / DRAW_SCREEN_HEIGHT;
	const float windowAspect = g_ClientWidth / g_ClientHeight;

	float vpW, vpH, vpX = 0.0f, vpY = 0.0f;

	if (windowAspect > targetAspect)
	{
		// 横長: 横いっぱい、縦がはみ出す
		vpW = g_ClientWidth;
		vpH = g_ClientWidth / targetAspect;
		vpY = (g_ClientHeight - vpH) * 0.5f;
	}
	else
	{
		// 縦長: 縦いっぱい、横がはみ出す
		vpH = g_ClientHeight;
		vpW = g_ClientHeight * targetAspect;
		vpX = (g_ClientWidth - vpW) * 0.5f;
	}

	// クライアントサイズとバックバッファサイズの比率差を補正
	const float scaleX = (float)g_BackBufferDesc.Width  / g_ClientWidth;
	const float scaleY = (float)g_BackBufferDesc.Height / g_ClientHeight;

	D3D11_VIEWPORT vp;
	vp.TopLeftX = vpX * scaleX;
	vp.TopLeftY = vpY * scaleY;
	vp.Width    = vpW * scaleX;
	vp.Height   = vpH * scaleY;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	g_ImmediateContext->RSSetViewports(1, &vp);
}


ID3D11Device* GetDevice( void )
{
	return g_D3DDevice;
}


ID3D11DeviceContext* GetDeviceContext( void )
{
	return g_ImmediateContext;
}


void SetDepthEnable( bool Enable )
{
	if( Enable )
	{
		g_ImmediateContext->OMSetDepthStencilState( g_DepthStateEnable, NULL );
		Direct3D_SetViewport3D();
	}
	else
	{
		g_ImmediateContext->OMSetDepthStencilState( g_DepthStateDisable, NULL );
		Direct3D_SetViewport2D();
	}
}

void ResetWorldViewProjection3D(void)
{
	//行列を単位行列にして初期化
	g_ProjectionMatrix = XMMatrixIdentity();
	g_ViewMatrix = XMMatrixIdentity();
	g_WorldMatrix = XMMatrixIdentity();
}

void SetWorldViewProjection2D( void )
{
	//2D用正射影行列をセット
	g_ProjectionMatrix = XMMatrixOrthographicOffCenterLH(0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f);
	SetProjectionMatrix(g_ProjectionMatrix);
	//行列を単位行列にして初期化
	g_ViewMatrix = XMMatrixIdentity();
	SetViewMatrix(g_ViewMatrix);
	g_WorldMatrix = XMMatrixIdentity();
	SetWorldMatrix(g_WorldMatrix);

}


void SetWorldMatrix( XMMATRIX WorldMatrix )
{
	XMMATRIX world;
	world = XMMatrixTranspose(WorldMatrix);
	XMFLOAT4X4 matrix;
	XMStoreFloat4x4(&matrix, world);
	g_ImmediateContext->UpdateSubresource(g_WorldBuffer, 0, NULL, &matrix, 0, 0);
}

void SetViewMatrix( XMMATRIX ViewMatrix )
{
	XMMATRIX view;
	view = XMMatrixTranspose(ViewMatrix);
	XMFLOAT4X4 matrix;
	XMStoreFloat4x4(&matrix, view);
	g_ImmediateContext->UpdateSubresource(g_ViewBuffer, 0, NULL, &matrix, 0, 0);
}

void SetProjectionMatrix( XMMATRIX ProjectionMatrix )
{
	XMMATRIX projection;
	projection = XMMatrixTranspose(ProjectionMatrix);
	XMFLOAT4X4 matrix;
	XMStoreFloat4x4(&matrix, projection);
	g_ImmediateContext->UpdateSubresource(g_ProjectionBuffer, 0, NULL, &matrix, 0, 0);
}



void SetMaterial( MATERIAL Material )
{

	GetDeviceContext()->UpdateSubresource( g_MaterialBuffer, 0, NULL, &Material, 0, 0 );

}

void SetCameraPosition(XMFLOAT3 CameraPosition)
{
	XMFLOAT4	temp = XMFLOAT4(CameraPosition.x, CameraPosition.y, CameraPosition.z, 0.0f);
	GetDeviceContext()->UpdateSubresource(g_CameraBuffer, 0, NULL, &temp, 0, 0);
}

void SetParameter(XMFLOAT4 Parameter)
{
	GetDeviceContext()->UpdateSubresource(g_ParameterBuffer, 0, NULL, &Parameter, 0, 0);
}

void SetShadowMatrix(XMMATRIX LightViewProjection, XMFLOAT4 Param)
{
	if (!g_ShadowBuffer) return;

	// HLSL側でmulしやすいように転置してからGPUへ送る。
	SHADOW_CONSTANT shadow = {};
	XMMATRIX lightViewProjection = XMMatrixTranspose(LightViewProjection);
	XMStoreFloat4x4(&shadow.LightViewProjection, lightViewProjection);
	shadow.Param = Param;
	shadow.Param.z = 1.0f / static_cast<float>(SHADOW_MAP_SIZE);
	shadow.Param.w = 1.0f / static_cast<float>(SHADOW_MAP_SIZE);

	g_ImmediateContext->UpdateSubresource(g_ShadowBuffer, 0, NULL, &shadow, 0, 0);
}

void BeginShadowMap(void)
{
	// 同じShadowMapを「読みながら書く」とDirectXで不正になるので、先に読み取りを外す。
	ID3D11ShaderResourceView* nullSRV = NULL;
	g_ImmediateContext->PSSetShaderResources(1, 1, &nullSRV);

	// RenderTargetViewを外し、深度だけを書くShadowMap用DepthViewへ切り替える。
	SetDepthEnable(true);
	g_ImmediateContext->OMSetRenderTargets(0, NULL, g_ShadowMapDepthView);
	g_ImmediateContext->ClearDepthStencilView(g_ShadowMapDepthView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// ShadowMapは画面サイズではなく、専用の固定サイズで描く。
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (FLOAT)SHADOW_MAP_SIZE;
	vp.Height = (FLOAT)SHADOW_MAP_SIZE;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	g_ImmediateContext->RSSetViewports(1, &vp);
}

void EndShadowMap(void)
{
	// ShadowMapへの描画を終えたので、通常の画面描画用RenderTargetへ戻す。
	g_ImmediateContext->OMSetRenderTargets(1, &g_RenderTargetView, g_DepthStencilView);
	SetDepthEnable(true);

	// 以降のピクセルシェーダーがShadowMapを読めるように、t1/s1へセットする。
	g_ImmediateContext->PSSetShaderResources(1, 1, &g_ShadowMapShaderView);
	g_ImmediateContext->PSSetSamplers(1, 1, &g_ShadowMapSampler);
}


void SetBlendState(BLENDSTATE blend)
{

	g_ImmediateContext->OMSetBlendState(bState[blend], bFactor, 0xffffffff);

}

void SetCullState(CULLSTATE cull)
{
	if (cull < 0 || cull >= CULLSTATE_MAX) return;
	g_ImmediateContext->RSSetState(rState[cull]);
}

//=============================================================================
// 初期化処理
//=============================================================================
HRESULT InitRenderer(HINSTANCE hInstance, HWND hWnd, BOOL bWindow)
{
	HRESULT hr = S_OK;

	// デバイス、スワップチェーン、コンテキスト生成
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount = 1;
	// スワップチェーンをウィンドウの実際のクライアント領域サイズで生成（ネイティブ解像度描画）
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	UINT initClientW = (UINT)(clientRect.right  - clientRect.left);
	UINT initClientH = (UINT)(clientRect.bottom - clientRect.top);
	if (initClientW == 0) initClientW = (UINT)SCREEN_WIDTH;
	if (initClientH == 0) initClientH = (UINT)SCREEN_HEIGHT;
	sd.BufferDesc.Width  = initClientW;
	sd.BufferDesc.Height = initClientH;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	hr = D3D11CreateDeviceAndSwapChain( NULL,
										D3D_DRIVER_TYPE_HARDWARE,
										NULL,
										0,
										NULL,
										0,
										D3D11_SDK_VERSION,
										&sd,
										&g_SwapChain,
										&g_D3DDevice,
										&g_FeatureLevel,
										&g_ImmediateContext );
	if( FAILED( hr ) )
		return hr;

	configureBackBuffer();

	// ラスタライザステート設定
	D3D11_RASTERIZER_DESC rd;
	ZeroMemory(&rd, sizeof(rd));
	rd.FillMode = D3D11_FILL_SOLID;
	rd.DepthClipEnable = TRUE;
	rd.MultisampleEnable = FALSE;

	D3D11_CULL_MODE cullMode[CULLSTATE_MAX] = {
		D3D11_CULL_NONE,
		D3D11_CULL_FRONT,
		D3D11_CULL_BACK
	};
	for (int i = 0; i < CULLSTATE_MAX; i++)
	{
		rd.CullMode = cullMode[i];
		g_D3DDevice->CreateRasterizerState(&rd, &rState[i]);
	}
	SetCullState(CULLSTATE_NONE);


	// ブレンドステート設定
	D3D11_BLEND_DESC blendDesc;
	ZeroMemory( &blendDesc, sizeof( blendDesc ) );
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	// BLENDSTATE_NONE: ブレンドなし
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	g_D3DDevice->CreateBlendState( &blendDesc, &bState[BLENDSTATE_NONE] );

	// BLENDSTATE_ALFA: 通常αブレンド
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
	g_D3DDevice->CreateBlendState( &blendDesc, &bState[BLENDSTATE_ALFA] );

	// BLENDSTATE_ADD: 加算合成
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOp   = D3D11_BLEND_OP_ADD;
	g_D3DDevice->CreateBlendState( &blendDesc, &bState[BLENDSTATE_ADD] );

	// BLENDSTATE_SUB: 減算合成
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_REV_SUBTRACT;
	g_D3DDevice->CreateBlendState( &blendDesc, &bState[BLENDSTATE_SUB] );

	// 初期ブレンドステートはαブレンド
	g_ImmediateContext->OMSetBlendState( bState[BLENDSTATE_ALFA], bFactor, 0xffffffff );


	// 深度ステンシルステート設定
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory( &depthStencilDesc, sizeof( depthStencilDesc ) );
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	g_D3DDevice->CreateDepthStencilState( &depthStencilDesc, &g_DepthStateEnable );//深度有効ステート

	//depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask	= D3D11_DEPTH_WRITE_MASK_ZERO;
	g_D3DDevice->CreateDepthStencilState( &depthStencilDesc, &g_DepthStateDisable );//深度無効ステート

	//深度テスト有効にしておく
	g_ImmediateContext->OMSetDepthStencilState( g_DepthStateEnable, NULL );

	// サンプラーステート設定
	D3D11_SAMPLER_DESC samplerDesc;
	ZeroMemory( &samplerDesc, sizeof( samplerDesc ) );
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;//ちょっといいフィルターにする
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;//横の座標範囲外は画像繰り返し
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;//縦の座標範囲外は画像繰り返し
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;//未使用
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 16;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	ID3D11SamplerState* samplerState = NULL;
	g_D3DDevice->CreateSamplerState( &samplerDesc, &samplerState );
	//サンプラーをシェーダーへセット
	g_ImmediateContext->PSSetSamplers( 0, 1, &samplerState );

	// ShadowMap本体。深度として書き込み、あとでテクスチャとして読むためTYPELESSで作る。
	D3D11_TEXTURE2D_DESC shadowTextureDesc;
	ZeroMemory(&shadowTextureDesc, sizeof(shadowTextureDesc));
	shadowTextureDesc.Width = SHADOW_MAP_SIZE;
	shadowTextureDesc.Height = SHADOW_MAP_SIZE;
	shadowTextureDesc.MipLevels = 1;
	shadowTextureDesc.ArraySize = 1;
	shadowTextureDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	shadowTextureDesc.SampleDesc.Count = 1;
	shadowTextureDesc.SampleDesc.Quality = 0;
	shadowTextureDesc.Usage = D3D11_USAGE_DEFAULT;
	shadowTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	g_D3DDevice->CreateTexture2D(&shadowTextureDesc, NULL, &g_ShadowMapTexture);

	// ShadowMapへ深度を書き込むためのView。
	D3D11_DEPTH_STENCIL_VIEW_DESC shadowDepthDesc;
	ZeroMemory(&shadowDepthDesc, sizeof(shadowDepthDesc));
	shadowDepthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	shadowDepthDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	g_D3DDevice->CreateDepthStencilView(g_ShadowMapTexture, &shadowDepthDesc, &g_ShadowMapDepthView);

	// ShadowMapをピクセルシェーダーで読むためのView。
	D3D11_SHADER_RESOURCE_VIEW_DESC shadowResourceDesc;
	ZeroMemory(&shadowResourceDesc, sizeof(shadowResourceDesc));
	shadowResourceDesc.Format = DXGI_FORMAT_R32_FLOAT;
	shadowResourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shadowResourceDesc.Texture2D.MipLevels = 1;
	g_D3DDevice->CreateShaderResourceView(g_ShadowMapTexture, &shadowResourceDesc, &g_ShadowMapShaderView);

	// ShadowMapを読むときのサンプラー。範囲外は影なし扱いにしやすいようBorderを白にする。
	D3D11_SAMPLER_DESC shadowSamplerDesc;
	ZeroMemory(&shadowSamplerDesc, sizeof(shadowSamplerDesc));
	shadowSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	shadowSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	shadowSamplerDesc.BorderColor[0] = 1.0f;
	shadowSamplerDesc.BorderColor[1] = 1.0f;
	shadowSamplerDesc.BorderColor[2] = 1.0f;
	shadowSamplerDesc.BorderColor[3] = 1.0f;
	shadowSamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	shadowSamplerDesc.MinLOD = 0.0f;
	shadowSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	g_D3DDevice->CreateSamplerState(&shadowSamplerDesc, &g_ShadowMapSampler);
	g_ImmediateContext->PSSetSamplers(1, 1, &g_ShadowMapSampler);


	//定数バッファ生成

	//================================================
	// WorldViewProjection行列用定数バッファ生成
	D3D11_BUFFER_DESC hBufferDesc;
	hBufferDesc.ByteWidth = sizeof(XMMATRIX);
	hBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	hBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hBufferDesc.CPUAccessFlags = 0;
	hBufferDesc.MiscFlags = 0;
	hBufferDesc.StructureByteStride = sizeof(float);
	//行列オブジェクトをシェーダーへ接続　b0をつかう
	g_D3DDevice->CreateBuffer(&hBufferDesc, NULL, &g_WorldBuffer);
	g_ImmediateContext->VSSetConstantBuffers(0, 1, &g_WorldBuffer);

	g_D3DDevice->CreateBuffer(&hBufferDesc, NULL, &g_ViewBuffer);
	g_ImmediateContext->VSSetConstantBuffers(1, 1, &g_ViewBuffer);

	g_D3DDevice->CreateBuffer(&hBufferDesc, NULL, &g_ProjectionBuffer);
	g_ImmediateContext->VSSetConstantBuffers(2, 1, &g_ProjectionBuffer);

	//マテリアル用定数バッファ生成
	hBufferDesc.ByteWidth = sizeof(MATERIAL);
	hBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	hBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hBufferDesc.CPUAccessFlags = 0;
	hBufferDesc.MiscFlags = 0;
	hBufferDesc.StructureByteStride = sizeof(float);
	//マテリアルオブジェクトをシェーダーへ接続　b1を使う
	g_D3DDevice->CreateBuffer( &hBufferDesc, NULL, &g_MaterialBuffer );
	g_ImmediateContext->VSSetConstantBuffers( 3, 1, &g_MaterialBuffer );

	hBufferDesc.ByteWidth = sizeof(LIGHT);

	g_D3DDevice->CreateBuffer(&hBufferDesc, NULL, &g_LightBuffer);
	g_ImmediateContext->VSSetConstantBuffers(4, 1, &g_LightBuffer);
	g_ImmediateContext->PSSetConstantBuffers(4, 1, &g_LightBuffer);

	hBufferDesc.ByteWidth = sizeof(XMFLOAT4);
	g_D3DDevice->CreateBuffer(&hBufferDesc, NULL, &g_CameraBuffer);
	g_ImmediateContext->PSSetConstantBuffers(5, 1, &g_CameraBuffer);

	g_D3DDevice->CreateBuffer(&hBufferDesc, NULL, &g_ParameterBuffer);
	g_ImmediateContext->VSSetConstantBuffers(6, 1, &g_ParameterBuffer);
	g_ImmediateContext->PSSetConstantBuffers(6, 1, &g_ParameterBuffer);
	SetParameter(XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f));

	// LightViewProjectionなど、ShadowMap判定に必要な値を入れる定数バッファ。
	hBufferDesc.ByteWidth = sizeof(SHADOW_CONSTANT);
	g_D3DDevice->CreateBuffer(&hBufferDesc, NULL, &g_ShadowBuffer);
	g_ImmediateContext->VSSetConstantBuffers(8, 1, &g_ShadowBuffer);
	g_ImmediateContext->PSSetConstantBuffers(8, 1, &g_ShadowBuffer);
	SetShadowMatrix(XMMatrixIdentity(), XMFLOAT4(0.003f, 0.55f, 0.0f, 0.0f));

	MATERIAL material;
	ZeroMemory(&material, sizeof(material));
	material.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	material.Ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	SetMaterial(material);


	return S_OK;
}


//=============================================================================
// 終了処理
//=============================================================================
void FinalizeRenderer(void)
{
	// オブジェクト解放
	if(g_WorldViewProjection)	g_WorldViewProjection->Release();
	if( g_MaterialBuffer )		g_MaterialBuffer->Release();
	if( g_VertexLayout )		g_VertexLayout->Release();
	if( g_VertexShader )		g_VertexShader->Release();
	if( g_PixelShader )			g_PixelShader->Release();
	SAFE_RELEASE(g_ShadowBuffer);
	SAFE_RELEASE(g_ShadowMapSampler);
	SAFE_RELEASE(g_ShadowMapShaderView);
	SAFE_RELEASE(g_ShadowMapDepthView);
	SAFE_RELEASE(g_ShadowMapTexture);
	for (int i = 0; i < CULLSTATE_MAX; i++)
	{
		SAFE_RELEASE(rState[i]);
	}

	if( g_ImmediateContext )	g_ImmediateContext->ClearState();
	releaseBackBuffer();
	if( g_SwapChain )			g_SwapChain->Release();
	if( g_ImmediateContext )	g_ImmediateContext->Release();
	if( g_D3DDevice )			g_D3DDevice->Release();
}


//=============================================================================
// バックバッファクリア
//=============================================================================
void Clear(void)
{
	// バックバッファクリア色
	float ClearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };//純黒は避ける
	//バックバッファをクリア
	g_ImmediateContext->ClearRenderTargetView( g_RenderTargetView, ClearColor );
	//デプスステンシルバッファをクリア
	g_ImmediateContext->ClearDepthStencilView( g_DepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);
}


//=============================================================================
// プレゼント
//=============================================================================
void Present(void)
{
	g_SwapChain->Present( 0, 0 );
}


// 頂点シェーダ生成
void CreateVertexShader(ID3D11VertexShader** VertexShader, ID3D11InputLayout** VertexLayout, const char* FileName)
{

	FILE* file;
	long int fsize;

	fopen_s(&file, FileName, "rb");
	if (file == NULL)
	{
		MessageBoxA(NULL, FileName, "Shader File Not Found (VS)", MB_OK);
		return;
	}
	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	g_D3DDevice->CreateVertexShader(buffer, fsize, NULL, VertexShader);

	// 入力レイアウト生成
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * 6, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 10, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE(layout);

	g_D3DDevice->CreateInputLayout(layout,
		numElements,
		buffer,
		fsize,
		VertexLayout);

	delete[] buffer;
}



// ピクセルシェーダ生成
void CreatePixelShader(ID3D11PixelShader** PixelShader, const char* FileName)
{
	FILE* file;
	long int fsize;

	fopen_s(&file, FileName, "rb");
	if (file == NULL)
	{
		MessageBoxA(NULL, FileName, "Shader File Not Found (PS)", MB_OK);
		return;
	}
	fsize = _filelength(_fileno(file));
	unsigned char* buffer = new unsigned char[fsize];
	fread(buffer, fsize, 1, file);
	fclose(file);

	g_D3DDevice->CreatePixelShader(buffer, fsize, NULL, PixelShader);

	delete[] buffer;
}

void SetLight(LIGHT Light)
{
	g_ImmediateContext->UpdateSubresource(g_LightBuffer, 0, NULL, &Light, 0, 0);
}

//=============================================================================
// ウィンドウクライアントサイズ通知（D3D リソースは変更しない）
//=============================================================================
void Direct3D_ResizeWindow(unsigned int clientW, unsigned int clientH)
{
	g_ClientWidth  = (clientW  > 0) ? (float)clientW  : 1.0f;
	g_ClientHeight = (clientH > 0) ? (float)clientH : 1.0f;
}

float Direct3D_GetClientWidth(void)
{
	return g_ClientWidth;
}

float Direct3D_GetClientHeight(void)
{
	return g_ClientHeight;
}

//=============================================================================
// バックバッファ/深度バッファの再構築（ウィンドウリサイズ時に明示的に呼ぶ）
//=============================================================================
void Direct3D_Resize(unsigned int width, unsigned int height)
{
	if (g_SwapChain == NULL || g_D3DDevice == NULL) return;
	if (width == 0 || height == 0) return;

	releaseBackBuffer();
	g_SwapChain->ResizeBuffers(1, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	configureBackBuffer();
}

