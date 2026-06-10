#include "Common.hlsl" //必須インクルード

void main(in VS_IN In, out PS_IN Out)
{
    Out = (PS_IN)0;

    // 頂点変換
    matrix wvp;
    wvp = mul(World, View);
    wvp = mul(wvp, Projection);
    Out.Position = mul(In.Position, wvp); // 投影空間への変換

    // 法線の回転 (並行移動させないためw=0)
    In.Normal.w = 0.0f;
    float4 worldNormal = mul(In.Normal, World);
    worldNormal = normalize(worldNormal);
    float4 worldPosition = mul(In.Position, World);
    float3 toLight = Light.Position.xyz - worldPosition.xyz;
    float distanceToLight = length(toLight);
    float3 lightDirection = (distanceToLight > 0.001f) ? toLight / distanceToLight : float3(0.0f, 1.0f, 0.0f);
    float lightRange = max(Light.PointLightParam.x, 0.001f);
    float lightIntensity = max(Light.PointLightParam.y, 0.0f);
    float attenuation = saturate(1.0f - distanceToLight / lightRange);
    attenuation = attenuation * attenuation * lightIntensity;

    // ランバート反射
    float light = 0.0f;
    if (Light.Enable)
    {
        light = saturate(dot(lightDirection, worldNormal.xyz)) * attenuation;
    }

    // 出力カラー計算 (頂点色×マテリアル色にライト乗算)
    float4 baseDiffuse = In.Diffuse * MaterialDiffuse;
    float3 ambient = saturate(Light.Ambient.rgb);
    float3 diffuse = light * baseDiffuse.rgb * Light.Diffuse.rgb;
    Out.Diffuse.rgb = saturate(baseDiffuse.rgb * ambient + diffuse);
    Out.Diffuse.a = baseDiffuse.a;

    Out.Normal = worldNormal;
    Out.TexCoord = In.TexCoord;
    Out.WorldPosition = worldPosition;
}
