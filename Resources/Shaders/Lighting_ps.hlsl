#include "PBS_helpers.hlsl"

struct CameraParams
{
	float4 CameraPos;
	matrix InvViewMatrix;
	matrix InvProjMatrix;
};

struct DirectionalLightParams
{
	float4 Direction;
	float4 Color;
};

struct PSInput
{
	float2 uv0 : TEXCOORD;
};

ConstantBuffer<CameraParams> CamCB : register(b0);
ConstantBuffer<DirectionalLightParams> DirectionalLightsCB[1] : register(b1);

Texture2D gb0 : register(t0); // (x,y,z) - baseColor RGB
Texture2D gb1 : register(t1); // (x,y,z) - normal XYZ
Texture2D gb2 : register(t2); // x - metall, y - roughness
Texture2D depthTexture : register(t3); // Depth 24-bit + Stencil 8-bit

SamplerState PointSampler : register(s0);

float3 GetWorldPosFromDepth(float depth, float2 uv)
{
	float z = depth;
	float x = uv.x * 2 - 1;
	float y = (1 - uv.y) * 2 - 1;

	float4 clipSpacePosition = float4(x, y, z, 1.0f);
	float4 viewSpacePosition = mul(CamCB.InvProjMatrix, clipSpacePosition);

	// Perspective division
	viewSpacePosition /= viewSpacePosition.w;

	float4 worldSpacePosition = mul(CamCB.InvViewMatrix, viewSpacePosition);

	return worldSpacePosition.xyz;
}

float LinearizeDepth(float depth)
{
	float nearZ = 0.01f;
	float farZ = 100.0f;
	return (2.0 * nearZ) / (farZ + nearZ - depth * (farZ - nearZ));
}

float4 main(PSInput input) : SV_TARGET
{
	float4 baseColor = gb0.Sample(PointSampler, input.uv0);
	float4 normal = gb1.Sample(PointSampler, input.uv0);
	float4 metalRoughness = gb2.Sample(PointSampler, input.uv0);

	float depth = depthTexture.Sample(PointSampler, input.uv0).r;
	float3 worldPos = GetWorldPosFromDepth(depth, input.uv0);

	float3 N = normal.xyz;
	float3 L = normalize(-DirectionalLightsCB[0].Direction.xyz);
	float3 V = normalize(CamCB.CameraPos.xyz - worldPos);

	// For each light
	float3 finalColor = BRDF(V, L, N, baseColor.xyz, metalRoughness.r, metalRoughness.g, DirectionalLightsCB[0].Color.rgb);

	return float4(finalColor, 1.0);
}