#include "Common.hlsl" //必須インクルード

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    // テクスチャサンプリング
    float4 texColor = g_Texture.Sample(g_SamplerState, In.TexCoord);

    // 頂点シェーダで計算されたライティング込みのDiffuseを掛ける
    outDiffuse = In.Diffuse * texColor;
}
