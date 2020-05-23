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

float G_SchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0f);
	float k = (r * r) / 8.0f;

	float num = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return num / denom;
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
	return F0 + (1.0f - F0) * pow(1.0f - VdotH, 5.0);
}