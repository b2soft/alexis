static const float k_pi = 3.14159265358979323846;
static const float k_invPi = 1.0 / k_pi;

float D_TrowbridgeReitz(float3 N, float3 H, float a)
{
	float a2 = a * a;

	float NdotH = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = k_pi * denom * denom;

	return nom / denom;
}

float G_SchlickGGX(float NdotV, float k)
{
	float nom = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return nom / denom;
}

float G_Smith(float3 N, float3 V, float3 L, float k)
{
	float NdotV = max(dot(N, V), 0.0f);
	float NdotL = max(dot(N, L), 0.0f);
	float ggx1 = G_SchlickGGX(NdotV, k);
	float ggx2 = G_SchlickGGX(NdotL, k);

	return ggx1 * ggx2;
}

float3 F_Schlick(float3 H, float3 V, float3 F0)
{
	float HdotV = max(dot(H, V), 0.0f);
	return F0 + (1.0f - F0) * pow(1.0f - HdotV, 5.0);
}