static const float k_pi = 3.14159265358979323846;
static const float k_invPi = 1.0 / k_pi;

float D_TrowbridgeReitz(float3 N, float3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;

	float NdotH = max(dot(N, H), 0.0f);
	float NdotH2 = NdotH * NdotH;

	float nom = a2;
	float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
	denom = k_pi * denom * denom;

	return nom / denom;
}

float G_SchlickGGX(float NdotV, float roughness)
{
	float a = roughness;
	float k = (a * a) / 2.0;

	float num = NdotV;
	float denom = NdotV * (1.0f - k) + k;

	return num / denom;
}

//float G_SchlickGGX(float NdotV, float roughness)
//{
//	float r = (roughness + 1.0f);
//	float k = (r * r) / 8.0f;
//
//	float num = NdotV;
//	float denom = NdotV * (1.0f - k) + k;
//
//	return num / denom;
//}

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
	return F0 + (1.0f - F0) * pow((1.0f - VdotH), 5.0);
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