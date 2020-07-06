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

struct PointLightParams
{
	float4 Position;
	float4 Color;
};

struct ShadowMapParams
{
	matrix LightSpaceMatrix;
};

struct PSInput
{
	float2 uv0 : TEXCOORD;
	float4 screenPos : SV_Position;
};

ConstantBuffer<CameraParams> CamCB : register(b0);
ConstantBuffer<DirectionalLightParams> DirectionalLightsCB[1] : register(b1);
ConstantBuffer<ShadowMapParams> ShadowMapCB : register(b2);
ConstantBuffer<PointLightParams> PointLightsCB : register(b4);

Texture2D gb0 : register(t0); // (x,y,z) - baseColor RGB
Texture2D gb1 : register(t1); // (x,y,z) - normal XYZ
Texture2D gb2 : register(t2); // x - metall, y - roughness
Texture2D depthTexture : register(t3); // Depth 24-bit + Stencil 8-bit

Texture2D shadowMap : register(t4); // Shadow Map

TextureCube irradianceMap : register(t5); // Irradiance Map
TextureCube prefilteredMap : register(t6); // Prefiltered Split-Sum Map
Texture2D brdfLUT : register(t7); // Convoluted BRDF LUT

SamplerState PointSampler : register(s0);
SamplerState LinearSampler : register(s1);
SamplerState LinearSamplerPointMip : register(s2);
SamplerState AnisoSampler : register(s3);

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
	//float2 uv = input.uv0;
	float2 uv = input.screenPos.xy / float2(1280, 720);// *float2(2,-2) - float2(1, -1);// / float2(1280, 720);
	//return float4(1.0, 0.0, 0.0, 1.0);
	float3 baseColor = gb0.Sample(PointSampler, uv).rgb;
	float3 normal = gb1.Sample(AnisoSampler, uv).xyz;
	float3 metalRoughness = gb2.Sample(PointSampler, uv).rgb;
	float metallic = metalRoughness.r;
	float roughness = metalRoughness.g;
	float ao = metalRoughness.b;

	float depth = depthTexture.Sample(PointSampler, uv).r;
	[flatten] if (depth >= 1.0f)
	{
		discard;
	}

	float3 worldPos = GetWorldPosFromDepth(depth, uv);

	//TODO: normal mapping
	float3 N = normalize(normal * 2.0 - 1.0);
	float3 V = normalize(CamCB.CameraPos.xyz - worldPos);
	float3 R = reflect(-V, N);

	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = lerp(F0, baseColor, metallic);

	float3 Lo = 0.0f;

	//[unroll]
	//for (int i = 0; i < 1; ++i)
	{
		float3 L = normalize(PointLightsCB.Position.xyz - worldPos);
		float3 H = normalize(V + L);

		float distance = length(PointLightsCB.Position.xyz - worldPos);
		float attenuation = 1 / (distance * distance);
		float3 radiance = PointLightsCB.Color.rgb * attenuation;

		// Cook-Torrance BRDF
		float NDF = D_TrowbridgeReitz(N, H, roughness);
		float G = G_Smith(N, V, L, roughness);
		float3 F = F_Schlick(dot(V,H), F0);

		float3 nom = NDF * G * F;
		float denom = 4.0f * max(dot(N, V), 0.0f) * max(dot(N, L), 0.0f) + 0.001f;
		float3 specular = nom / denom;

		// Energy conservation kS + kD = 1.0
		float3 kS = F;
		float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;

		// Multiply kD by the inv metalness because only non-metals have diffuse.
		// Pure metals have diffuse only
		kD *= (1.0f - metallic);

		float NdotL = max(dot(N, L), 0.0f);
		Lo += (kD * baseColor * k_invPi + specular) * radiance * NdotL;
	}


	float3 ambient = 0.f;
	// Env BRDF
	//{
	//	float3 F = F_SchlickRoughness(max(dot(N, V), 0.0f), F0, roughness);
	//
	//	float3 kS = F;
	//	float3 kD = 1.0 - kS;
	//	kD *= (1.0 - metallic);
	//
	//	float3 irradiance = irradianceMap.Sample(LinearSampler, N).rgb;
	//	float3 diffuse = irradiance * baseColor;
	//
	//	const float k_maxReflectionLod = 5.0f;
	//	float3 prefilteredColor = prefilteredMap.SampleLevel(LinearSampler, R, roughness * k_maxReflectionLod).rgb;
	//	float2 envBRDF = brdfLUT.Sample(LinearSampler, float2(max(dot(N,V), 0.0), roughness)).rg;
	//	float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
	//
	//	ambient = (kD * diffuse + specular) * ao;
	//}

	float3 color = ambient + Lo;
	//float3 color = NDF;// ambient + Lo;

	//return float4(N, 1.0);

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
}