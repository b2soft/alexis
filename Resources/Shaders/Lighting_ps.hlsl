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

ConstantBuffer<SunLight> SunCB : register(b2);

Texture2D gb0 : register(t0); // (x,y,z) - baseColor, a - worldPos x
Texture2D gb1 : register(t1); // (x,y,z) - normal, a - worldPos y
Texture2D gb2 : register(t2); // x - metall, y - roughness, a - worldPos z

SamplerState PointSampler : register(s0);

// float3 BRDF(float3 v, float3 l, float3 n, float perceptualRoughness, float f0, float3 diffuseColor)
float4 main(PSInput input) : SV_TARGET
{
	float4 baseColor = gb0.Sample(PointSampler, input.uv0);
	float4 normal = gb1.Sample(PointSampler, input.uv0);
	float4 metalRoughness = gb2.Sample(PointSampler, input.uv0);
	float3 worldPos = float3(baseColor.a, normal.a, metalRoughness.a);

	float3 V = normalize(SunCB.ViewPos.xyz - worldPos);

	float3 N = normal.xyz;
	float3 L = -SunCB.Parameters.xyz;

	float3 sunColor = float3(1.0, 1.0, 1.0);

	float3 finalColor = BRDF(V, L, N, metalRoughness.y, metalRoughness.x, baseColor.xyz);
	finalColor = finalColor * sunColor * SunCB.Parameters.w * dot(N,L);

	//float intensity = dot(N, L);
	//float3 finalColor = saturate(diffuse * intensity);
	//finalColor = finalColor * baseColor;

	//return float4(N, 1.0);
	return float4(finalColor, 1.0);
}