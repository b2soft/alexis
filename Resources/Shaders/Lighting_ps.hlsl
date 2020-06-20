#include "PBS_helpers.hlsli"

#define USE_PCF9

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

	float3 lightPos = float3(21.0, 5.0, 17.0);
	//float3 lightColor = float3(70.0f, 70.0f, 70.0f);
	float3 lightColor = float3(5, 5, 5);

	float3 N = normal.xyz * 2.0f - 1.0f;
	//float3 L = normalize(DirectionalLightsCB[0].Direction.xyz - worldPos);
	float3 V = normalize(CamCB.CameraPos.xyz - worldPos);

	[flatten] if (depth >= 1.0f)
	{
		discard;
	}

	float metalness = metalRoughness.r;
	float roughness = metalRoughness.g;

	float3 Lo = 0.0f;

	// For each light
	float3 L = normalize(lightPos - worldPos);
	float3 H = normalize(V + L);

	float distance = length(lightPos - worldPos);
	float attenuation = 1.0;// / (distance * distance);
	float3 radiance = lightColor * attenuation;

	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = lerp(F0, baseColor.rgb, metalness);
	float3 F = F_Schlick(max(dot(H, V), 0.0f), F0);

	float D = D_TrowbridgeReitz(N, H, roughness);
	float G = G_Smith(N, V, L, roughness);

	float3 num = D * G * F;
	float denom = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.001f;
	float3 specular = num / denom;

	float3 kS = F;
	float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;

	kD *= 1.0f - metalness;

	float NdotL = max(dot(N, L), 0.0f);
	Lo += (kD * baseColor.rgb * k_invPi + specular) * radiance * NdotL;

	float ao = 1.0f;
	float3 ambient = float3(0.03f, 0.03f, 0.03f) * baseColor.rgb * ao;
	float3 color = ambient + Lo;

	return float4(color, 1.0f);

	//return float4(1.0, 0.0, 0.0, 1.0);

	//return float4(F, 1.0);
	//return float4(G, G, G, 1.0);

	//return saturate(dot(N, L));

	// For each light
//	float3 finalColor = BRDF(V, L, N, baseColor.xyz, metalRoughness.r, metalRoughness.g, DirectionalLightsCB[0].Color.rgb);
//
//	float3 ambient = baseColor.rgb * 0.15f; // fake indirect
//
//	float4 lightSpacePos = mul(ShadowMapCB.LightSpaceMatrix, float4(worldPos, 1.0));
//
//	lightSpacePos.xyz /= lightSpacePos.w;
//
//	float2 depthUV = lightSpacePos.xy;
//	depthUV.x = depthUV.x * 0.5 + 0.5;
//	depthUV.y = -depthUV.y * 0.5 + 0.5;
//
//	float depthFromMap = shadowMap.SampleLevel(ShadowSampler, depthUV, 0).r;
//	float pointDepth = lightSpacePos.z;
//
//	float bias = max(0.01 * (1.0 - dot(N, L)), 0.005);
//
//#if defined(USE_PCF9)
//	//PCF 9 shadows
//	float shadow = 0.0;
//	float2 texelSize = 1.0 / 1024.0;
//	for (int x = -1; x <= 1; ++x)
//	{
//		for (int y = -1; y <= 1; ++y)
//		{
//			float pcfDepth = shadowMap.SampleLevel(ShadowSampler, depthUV + float2(x, y) * texelSize, 0).r;
//			shadow += pointDepth - bias > pcfDepth ? 1.0 : 0.0;
//		}
//	}
//	shadow /= 9.0;
//#else
//	float shadow = pointDepth - bias > depthFromMap ? 1.0 : 0.0;
//#endif
//
//	finalColor = ambient + finalColor * (1.0 - shadow);

	//return float4(finalColor, 1.0);
	return 1.0;
}