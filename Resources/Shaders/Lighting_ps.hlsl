#include "PBS_helpers.hlsl"

struct PSInput
{
	float2 uv0 : TEXCOORD;
};


struct SunLight
{
	float4 Parameters; // (x,y,z) - direction, w - intensity
	float4 ViewPos;
};

ConstantBuffer<SunLight> SunCB : register(b0);

Texture2D gb0 : register(t0); // (x,y,z) - baseColor, worldPos x
Texture2D gb1 : register(t1); // (x,y,z) - normal, worldPos y
Texture2D gb2 : register(t2); // x - metall, y - roughness, a - worldPos z

SamplerState PointSampler : register(s0);

// float3 BRDF(float3 v, float3 l, float3 n, float perceptualRoughness, float f0, float3 diffuseColor)
float4 main(PSInput input) : SV_TARGET
{
	float4 col = gb0.Sample(PointSampler, input.uv0);
	float4 normal = gb1.Sample(PointSampler, input.uv0);
	float4 metalRoughness = gb2.Sample(PointSampler, input.uv0);
	float3 worldPos = float3(col.a, normal.a, metalRoughness.a);

	float3 viewDir = normalize(SunCB.ViewPos.xyz - worldPos);

	float3 finalColor = BRDF(viewDir, SunCB.Parameters.xyz, normal.xyz, metalRoughness.y, metalRoughness.x, col.xyz);

	return float4(worldPos, 1.0);
	//return finalColor;
}