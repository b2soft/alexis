static const float k_pi = 3.14159265358979323846;
static const float k_invPi = 1.0 / k_pi;

float D_TrowbridgeReitz(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float NdotH = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float num = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = k_pi * denom * denom;

	return num / denom;
}

float G_SchlickGGX_IBL(float NdotV, float roughness)
{
	float a = roughness;
	float k = (a * a) / 2.0;

	float num = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return num / denom;
}

float G_SchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0f);
	float k = (r * r) / 8.0f;

	float num = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return num / denom;
}

float G_Smith_IBL(float3 N, float3 V, float3 L, float roughnesss)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	float ggx1 = G_SchlickGGX_IBL(NdotV, roughnesss);
	float ggx2 = G_SchlickGGX_IBL(NdotL, roughnesss);

	return ggx1 * ggx2;
}

float G_Smith(float3 N, float3 V, float3 L, float roughnesss)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	float ggx1 = G_SchlickGGX(NdotV, roughnesss);
	float ggx2 = G_SchlickGGX(NdotL, roughnesss);

	return ggx1 * ggx2;
}

float3 F_Schlick(float VdotH, float3 F0)
{
	return F0 + (1.0f - F0) * pow(max(1.0f - VdotH, 0.0), 5.0);
}

float3 F_SchlickRoughness(float VdotH, float3 F0, float roughness)
{
	return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(1.0f - VdotH, 5.0);
}

float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
	float a = roughness * roughness;

	float phi = 2.0 * k_pi * Xi.x;
	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	// Spherical To Cartesian
	float3 H;
	H.x = cos(phi) * sinTheta;
	H.y = sin(phi) * sinTheta;
	H.z = cosTheta;

	// Tangent to World Space
	float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 tangent = normalize(cross(up, N));
	float3 bitangent = cross(N, tangent);

	float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
	return normalize(sampleVec);
}

struct BRDFInput
{
	float3 BaseColor;
	float3 N;
	float3 V;
	float3 L;
	float Metallic;
	float Roughness;
};

struct BRDFOutput
{
	float3 Diffuse;
	float3 Specular;
};

BRDFOutput BRDF(BRDFInput input)
{
	float3 H = normalize(input.V + input.L);
	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = lerp(F0, input.BaseColor, input.Metallic);

	// Cook-Torrance BRDF
	float NDF = D_TrowbridgeReitz(input.N, H, input.Roughness);
	float G = G_Smith(input.N, input.V, input.L, input.Roughness);
	float3 F = F_Schlick(max(dot(H, input.V), 0.0), F0);

	float NdotL = max(dot(input.N, input.L), 0.0f);

	float3 nom = NDF * G * F;
	float denom = 4.0f * max(dot(input.N, input.V), 0.0f) * NdotL;
	float3 specular = nom / max(denom, 0.001f);

	// Energy conservation kS + kD = 1.0
	float3 kS = F;
	float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;

	// Multiply kD by the inv metalness because only non-metals have diffuse.
	// Pure metals have diffuse only
	kD *= (1.0f - input.Metallic);

	float3 diffuse = kD * input.BaseColor * k_invPi;

	BRDFOutput output;
	output.Diffuse = diffuse;
	output.Specular = specular;

	return output;
}

struct EnvBRDFInput
{
	float3 BaseColor;
	float3 N;
	float3 V;
	float Metallic;
	float Roughness;
	TextureCube IrradianceMap;
	TextureCube PrefilteredMap;
	Texture2D BrdfLUT;
	SamplerState LinearSampler;
};

struct EnvBRDFOutput
{
	float3 Diffuse;
	float3 Specular;
};

EnvBRDFOutput EnvBRDF(EnvBRDFInput input)
{
	float3 F0 = float3(0.04f, 0.04f, 0.04f);
	F0 = lerp(F0, input.BaseColor, input.Metallic);

	float NdotV = max(dot(input.N, input.V), 0.0f);
	float3 R = reflect(-input.V, input.N);

	float3 F = F_SchlickRoughness(NdotV, F0, input.Roughness);

	float3 kS = F;
	float3 kD = 1.0 - kS;
	kD *= (1.0 - input.Metallic);

	float3 irradiance = input.IrradianceMap.Sample(input.LinearSampler, input.N).rgb;
	float3 diffuse = kD * irradiance * input.BaseColor;

	const float k_maxReflectionLod = 5.0f;
	float3 prefilteredColor = input.PrefilteredMap.SampleLevel(input.LinearSampler, R, input.Roughness * k_maxReflectionLod).rgb;
	float2 envBRDF = input.BrdfLUT.Sample(input.LinearSampler, float2(NdotV, input.Roughness)).rg;
	float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

	EnvBRDFOutput output;
	output.Diffuse = diffuse;
	output.Specular = specular;

	return output;
}