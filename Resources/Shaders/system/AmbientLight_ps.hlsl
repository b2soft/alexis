#include "../utils/Common.hlsli"
#include "../utils/PBSHelpers.hlsli"

struct PSInput
{
	float2 uv0 : TEXCOORD;
};

struct CameraParams
{
	float4 CameraPos;
	matrix InvViewMatrix;
	matrix InvProjMatrix;
};

ConstantBuffer<CameraParams> CamCB : register(b0);

Texture2D gb0 : register(t0); // (x,y,z) - baseColor RGB
Texture2D gb1 : register(t1); // (x,y,z) - normal XYZ
Texture2D gb2 : register(t2); // x - metall, y - roughness
Texture2D depthTexture : register(t3); // Depth 24-bit + Stencil 8-bit
TextureCube irradianceMap : register(t4); // Irradiance Map
TextureCube prefilteredMap : register(t5); // Prefiltered Split-Sum Map
Texture2D brdfLUT : register(t6); // Convoluted BRDF LUTt

SamplerState AnisoSampler : register(s0);
SamplerState LinearSampler : register(s1);

float4 main(PSInput input) : SV_TARGET
{
	float2 uv = input.uv0;
	float3 baseColor = gb0.Sample(AnisoSampler, uv).rgb;
	float3 normal = gb1.Sample(AnisoSampler, uv).xyz;
	float3 metalRoughness = gb2.Sample(AnisoSampler, uv).rgb;
	float metallic = metalRoughness.r;
	float roughness = metalRoughness.g;
	float ao = metalRoughness.b;

	float depth = depthTexture.Sample(AnisoSampler, uv).r;
	[flatten] if (depth >= 1.0f)
	{
		discard;
	}

	float3 worldPos = GetWorldPosFromDepth(depth, uv, CamCB.InvViewMatrix, CamCB.InvProjMatrix);

	float3 N = normalize(normal * 2.0 - 1.0);
	float3 V = normalize(CamCB.CameraPos.xyz - worldPos);

	EnvBRDFInput envBrdfInput;
	envBrdfInput.BaseColor = baseColor;
	envBrdfInput.N = N;
	envBrdfInput.V = V;
	envBrdfInput.Metallic = metallic;
	envBrdfInput.Roughness = roughness;
	envBrdfInput.IrradianceMap = irradianceMap;
	envBrdfInput.PrefilteredMap = prefilteredMap;
	envBrdfInput.BrdfLUT = brdfLUT;
	envBrdfInput.LinearSampler = LinearSampler;

	EnvBRDFOutput envBrdf = EnvBRDF(envBrdfInput);
	float3 color = (envBrdf.Diffuse + envBrdf.Specular) * ao;

	return float4(color, 1.0f);
}