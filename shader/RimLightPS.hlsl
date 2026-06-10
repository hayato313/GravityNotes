#include "Common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_SamplerState : register(s0);

void main(in PS_IN In, out float4 outDiffuse : SV_Target)
{
    // リムライトは、面の法線とカメラ方向の角度で強さが変わる
    float3 normal = normalize(In.Normal.xyz);
    float3 viewDir = normalize(CameraPosition.xyz - In.WorldPosition.xyz);

    // 輪郭に近いほど強く、正面を向いている面では弱くする
    float rim = 1.0f - saturate(dot(normal, viewDir));
    rim = smoothstep(0.35f, 0.95f, rim);

    // Parameter.x でC++側から全体の明るさを調整する
    float4 texColor = g_Texture.Sample(g_SamplerState, In.TexCoord);
    float brightness = max(Parameter.x, 0.0f);
    float3 baseColor = texColor.rgb * In.Diffuse.rgb;

    // 輪郭部分だけに青白いハイライトを足す
    float3 rimColor = float3(0.55f, 0.85f, 1.0f) * rim * 0.55f;

    outDiffuse.rgb = saturate((baseColor + rimColor) * brightness);
    outDiffuse.a = texColor.a * In.Diffuse.a;
}
