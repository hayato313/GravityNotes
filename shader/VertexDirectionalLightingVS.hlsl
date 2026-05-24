#include "Common.hlsl" //必須インクルード

void main(in VS_IN In, out PS_IN Out)
{
    // 頂点変換
    matrix wvp;
    wvp = mul(World, View);
    wvp = mul(wvp, Projection);
    Out.Position = mul(In.Position, wvp); // 投影空間への変換

    // 法線の回転 (並行移動させないためw=0)
    In.Normal.w = 0.0f;
    float4 worldNormal = mul(In.Normal, World);
    worldNormal = normalize(worldNormal);

    // ランバート反射
    float light = -dot(Light.Direction.xyz, worldNormal.xyz);
    light = saturate(light);

    // 出力カラー計算 (Diffuseにライト乗算)
    Out.Diffuse.rgb = light * In.Diffuse.rgb;
    Out.Diffuse.a = In.Diffuse.a;

    Out.Normal = worldNormal;
    Out.TexCoord = In.TexCoord;
    Out.WorldPosition = mul(In.Position, World);
}
