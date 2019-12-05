static const float k_pi = 3.14159265358979323846;
static const float k_invPi = 1.0 / k_pi;

//https://cdn2.unrealengine.com/Resources/files/2013SiggraphPresentationsNotes-26915738.pdf
// Cook-Torrance Specular BRDF
// Normal Distribution Function (D Term)
// Disney's GGX/Towbridge-Reitz
// a = roughness^2
float D_GGX(float NdotH, float a)
{
	float a2 = a * a;
	float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
	return (a2 * k_invPi) / (denom * denom);
}

// Geometric Shadowing (G Term)
// Schlick model with k = a/2
float G_Schlick(float NdotL, float NdotV, float roughness)
{
	float k = (roughness + 1.0f) * (roughness + 1.0f) * 0.125f;
	float GGX_L = NdotL / (NdotL * (1.0f - k) + k);
	float GGX_V = NdotV / (NdotV * (1.0f - k) + k);
	return GGX_V * GGX_L;
}

// Fresnel (F Term)
float3 F_Schlick(float3 f0, float VdotH)
{
	return f0 + (1.0f - f0) * exp2((-5.55473 * VdotH - 6.98316) * VdotH);
}

// Diffuse BRDF
// Lambertian BRDF
float Fd_Lambert()
{
	return k_invPi;
}

float3 BRDF(float3 v, float3 l, float3 n, float3 baseColor, float metallic, float roughness)
{
	baseColor = float3(1.0, 0.0, 0.0);
	metallic = 0.0f;
	roughness = 0.1f;

	float3 lightColor = float3(1.0, 1.0, 1.0);
	float3 h = normalize(l + v);

	float NdotV = abs(dot(n, v)) + 1e-5f;
	float NdotL = saturate(dot(n, l));
	float NdotH = saturate(dot(n, h));
	float VdotH = saturate(dot(v, h));

	float3 diffuseColor = (1.0 - metallic) * baseColor.rgb;
	float3 specularColor = lerp(0.08, baseColor.rgb, metallic);

	float3 diffuse = diffuseColor * Fd_Lambert();

	float a = roughness * roughness;
	float D = D_GGX(NdotH, a);
	float G = G_Schlick(NdotL, NdotV, roughness);
	float3 F = F_Schlick(specularColor, VdotH);

	float3 specular = (D * F * G) / (4.0f * NdotL * NdotV);

	float3 finalColor = NdotL * (diffuse + specular);

	return finalColor;
}