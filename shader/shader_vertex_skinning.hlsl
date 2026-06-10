//===============================================================================================
//
// AnimSprite3D用スキニングシェーダー
// 頂点をボーンに従って自動計算で動かすためのものなので、Shadermanagerには絶対に含めない
//
//===============================================================================================

#include "Common.hlsl"

#define MAX_BONES 256

cbuffer BoneBuffer : register(b7)
{
	row_major float4x4 BoneMatrices[MAX_BONES];
};

struct VS_IN_SKINNED
{
	float4 Position   : POSITION0;
	float4 Normal     : NORMAL0;
	float4 Diffuse    : COLOR0;
	float2 TexCoord   : TEXCOORD0;
	uint4  BoneIndex  : BLENDINDICES0;
	float4 BoneWeight : BLENDWEIGHT0;
};

void main(in VS_IN_SKINNED In, out PS_IN Out)
{
	Out = (PS_IN)0;

	float4 skinnedPos = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 skinnedNormal = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float totalWeight = In.BoneWeight.x + In.BoneWeight.y + In.BoneWeight.z + In.BoneWeight.w;

	if (totalWeight > 0.0f)
	{
		[unroll]
		for (int i = 0; i < 4; i++)
		{
			float w = In.BoneWeight[i];
			uint idx = In.BoneIndex[i];
			if (w > 0.0f && idx < MAX_BONES)
			{
				skinnedPos += w * mul(In.Position, BoneMatrices[idx]);
				skinnedNormal += w * mul(float4(In.Normal.xyz, 0.0f), BoneMatrices[idx]);
			}
		}
		skinnedPos.w = 1.0f;
	}
	else
	{
		skinnedPos = In.Position;
		skinnedNormal = float4(In.Normal.xyz, 0.0f);
	}

	matrix wvp = mul(World, View);
	wvp = mul(wvp, Projection);

	Out.Position = mul(skinnedPos, wvp);
	Out.WorldPosition = mul(skinnedPos, World);
	Out.Normal = normalize(mul(float4(skinnedNormal.xyz, 0.0f), World));
	Out.Diffuse = In.Diffuse;
	Out.TexCoord = In.TexCoord;
}
