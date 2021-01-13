#include "../utils/Common.hlsli"
#include "../utils/PBSHelpers.hlsli"

struct PointLightParams
{
	float4 Position;
	float4 Color;
};

struct ScreenParams
{
	float4 Params; // xy - Screen Size, zw - Inverted Screen Size
};

struct CameraParams
{
	float4 CameraPos;
	matrix InvViewMatrix;
	matrix InvProjMatrix;
};

ConstantBuffer<PointLightParams> PointLightCB : register(b1);
ConstantBuffer<ScreenParams> ScreenCB : register(b2);
ConstantBuffer<CameraParams> CamCB : register(b3);

Texture2D gb0 : register(t0); // (x,y,z) - baseColor RGB
Texture2D gb1 : register(t1); // (x,y,z) - normal XYZ
Texture2D gb2 : register(t2); // x - metall, y - roughness
Texture2D depthTexture : register(t3); // Depth 24-bit + Stencil 8-bit

SamplerState AnisoSampler : register(s0);

float4 main(float4 screenPos : SV_Position) : SV_TARGET
{
	float2 uv = screenPos.xy * ScreenCB.Params.zw;
	float3 baseColor = gb0.Sample(AnisoSampler, uv).rgb;
	float3 normal = gb1.Sample(AnisoSampler, uv).xyz;
	float3 metalRoughness = gb2.Sample(AnisoSampler, uv).rgb;
	float metallic = metalRoughness.r;
	float roughness = max(metalRoughness.g, 0.05);
	float ao = metalRoughness.b;

	float depth = depthTexture.Sample(AnisoSampler, uv).r;
	[flatten] if (depth >= 1.0f)
	{
		discard;
	}

	float3 worldPos = GetWorldPosFromDepth(depth, uv, CamCB.InvViewMatrix, CamCB.InvProjMatrix);

	float3 N = normalize(normal * 2.0 - 1.0);
	float3 V = normalize(CamCB.CameraPos.xyz - worldPos);
	float3 R = reflect(-V, N);
	float3 L = normalize(PointLightCB.Position.xyz - worldPos);

	BRDFInput brdfInput;
	brdfInput.BaseColor = baseColor;
	brdfInput.N = N;
	brdfInput.V = V;
	brdfInput.L = L;
	brdfInput.Metallic = metallic;
	brdfInput.Roughness = roughness;

	float distance = length(PointLightCB.Position.xyz - worldPos);
	float attenuation = 1.0 / (distance * distance);
	float3 radiance = PointLightCB.Color.rgb * attenuation;

	BRDFOutput brdf = BRDF(brdfInput);

	float3 color = (brdf.Diffuse + brdf.Specular) * (radiance * max(dot(N,L), 0.0));

	return float4(color, 1.0f);
}