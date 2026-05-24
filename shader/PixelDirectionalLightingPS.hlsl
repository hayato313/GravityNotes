#include "Common.hlsl" //必須インクルード

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    // 法線の正規化 (補間によって縮んでいるため)
    float3 worldNormal = normalize(In.Normal.xyz);

    // ランバート反射
    float light = -dot(Light.Direction.xyz, worldNormal);
    light = saturate(light);

    // スペキュラー反射（ブリン・フォン）
    float3 L = normalize(-Light.Direction.xyz);
    float3 V = normalize(CameraPosition.xyz - In.WorldPosition.xyz);
    float3 H = normalize(L + V);
    float specular = pow(saturate(dot(worldNormal, H)), 50.0);

    // テクスチャサンプリング
    float4 texColor = g_Texture.Sample(g_SamplerState, In.TexCoord);

    // 最終出力 (ランバート + スペキュラー)
    outDiffuse.rgb = texColor.rgb * In.Diffuse.rgb * light + specular * Light.Diffuse.rgb;
    outDiffuse.a = texColor.a * In.Diffuse.a;
}
