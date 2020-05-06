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

struct ShadowMapParams
{
	matrix LightSpaceMatrix;
};

struct PSInput
{
	float2 uv0 : TEXCOORD;
};

ConstantBuffer<CameraParams> CamCB : register(b0);
ConstantBuffer<DirectionalLightParams> DirectionalLightsCB[1] : register(b1);
ConstantBuffer<ShadowMapParams> ShadowMapCB : register(b2);

Texture2D gb0 : register(t0); // (x,y,z) - baseColor RGB
Texture2D gb1 : register(t1); // (x,y,z) - normal XYZ
Texture2D gb2 : register(t2); // x - metall, y - roughness
Texture2D depthTexture : register(t3); // Depth 24-bit + Stencil 8-bit

Texture2D shadowMap : register(t4); // Shadow Map

SamplerState PointSampler : register(s0);
SamplerState ShadowSampler : register(s1);

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

	float3 ambient = baseColor.rgb * 0.15f; // fake indirect

	float4 lightSpacePos = mul(ShadowMapCB.LightSpaceMatrix, float4(worldPos, 1.0));
	//return lightSpacePos.w;
	lightSpacePos.xyz /= lightSpacePos.w;
	lightSpacePos.z = 0.5 * lightSpacePos.z + 0.5;

	float2 depthUV = lightSpacePos.xy;
	depthUV.x = depthUV.x * 0.5 + 0.5;
	depthUV.y = -depthUV.y * 0.5 + 0.5;

	float depthFromMap = shadowMap.Sample(ShadowSampler, depthUV).r;
	float pointDepth = lightSpacePos.z;

	float bias = max(0.05 * (1.0 - dot(N, L)), 0.005);
	float shadow = pointDepth - bias > depthFromMap ? 1.0 : 0.0;
	
	//return float4(input.uv0, 0.0, 1.0);

	//return depthFromMap;

	//if (depthUV.z > 1.0)
	//	shadow = 0.0f;

	finalColor = ambient + finalColor * (1.0 - shadow);

	return float4(finalColor, 1.0);
}