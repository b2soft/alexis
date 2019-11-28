#include "PBS_helpers.hlsl"

struct CameraParams
{
	matrix InvViewMatrix;
	matrix InvProjMatrix;
};

struct PSInput
{
	float2 uv0 : TEXCOORD;
};


struct SunLight
{
	float4 Parameters; // (x,y,z) - direction, w - intensity
	float4 ViewPos;
};

ConstantBuffer<CameraParams> CamCB : register(b0);
ConstantBuffer<SunLight> SunCB : register(b2);

Texture2D gb0 : register(t0); // (x,y,z) - baseColor
Texture2D gb1 : register(t1); // (x,y,z) - normal
Texture2D gb2 : register(t2); // x - metall
Texture2D depthTexture : register(t3); // Depth

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

// float3 BRDF(float3 v, float3 l, float3 n, float perceptualRoughness, float f0, float3 diffuseColor)
float4 main(PSInput input) : SV_TARGET
{
	float4 baseColor = gb0.Sample(PointSampler, input.uv0);
	float4 normal = gb1.Sample(PointSampler, input.uv0);
	float4 metalRoughness = gb2.Sample(PointSampler, input.uv0);

	float depth = depthTexture.Sample(PointSampler, input.uv0).r;
	float3 worldPos = GetWorldPosFromDepth(depth, input.uv0);

	float3 V = normalize(SunCB.ViewPos.xyz - worldPos);

	float3 N = normalize(normal.xyz);
	float3 L = normalize(-SunCB.Parameters.xyz);

	float3 sunColor = float3(1.0, 1.0, 1.0);


	//return LinearizeDepth(depth);
	//return float4(V, 1.0);

	float3 finalColor = BRDF(V, L, N, metalRoughness.y, metalRoughness.x, baseColor.xyz);
	//finalColor = finalColor * sunColor * SunCB.Parameters.w * dot(N,L);

	//float intensity = dot(N, L);
	//float3 finalColor = saturate(diffuse * intensity);
	//finalColor = finalColor * baseColor;

	//return float4(N, 1.0);
	return float4(finalColor, 1.0);
}