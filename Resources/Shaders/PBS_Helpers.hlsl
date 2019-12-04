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
	//float f = (NdotH * a2 - NdotH) * NdotH + 1.0;
	float f = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
	return (a2 * k_invPi) / (f * f);
}

// Geometric Shadowing (G Term)
// Schlick model with k = a/2
float G_Schlick(float NdotL, float NdotV, float roughness)
{
	float k = (roughness + 1.0f) * (roughness + 1.0f) * 0.125f;
	float GGX_V = NdotV / (NdotV * (1.0 - k) + k);
	float GGX_L = NdotL / (NdotL * (1.0 - k) + k);
	return GGX_V + GGX_L;
}

// Fresnel (F Term)
float3 F_Schlick(float3 f0, float VdotH)
{
	float p = (-5.55473f * VdotH - 6.98316f) * VdotH;
	return f0 + (1 - f0) * pow(2.0f, p);
}

// Geometric Shadowing (G Term)
// a2 is roughness squared
float V_SmithGGXCorrelated(float NdotL, float NdotV, float a)
{
	float a2 = a * a;
	float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
	float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);

	return 0.5 / max(0.0001f, GGXV + GGXL);
}

// Diffuse BRDF
// Lambertian BRDF
float Fd_Lambert()
{
	return k_invPi;
}

float3 BRDF(float3 v, float3 l, float3 n, float3 baseColor, float metallic, float roughness)
{
	float3 lightColor = float3(1.0, 1.0, 1.0);
	float3 h = normalize(l + v);

	float NdotV = abs(dot(n, v)) + 1e-5f;
	float NdotL = saturate(dot(n, l));
	float NdotH = saturate(dot(n, h));
	float VdotH = saturate(dot(v, h));

	float3 diffuseColor = (1.0 - metallic) * baseColor.rgb;
	float3 specularColor = lerp(0.04, baseColor.rgb, metallic);

	float D = D_GGX(NdotH, roughness * roughness);
	float3 F = F_Schlick(specularColor, VdotH);
	float G = G_Schlick(NdotL, NdotV, roughness);

	float3 diffuse = diffuseColor * Fd_Lambert() * lightColor * NdotL;

	float3 specularBRDF = (D * F * G) / (4.0f * NdotL * NdotV);
	float3 specular = specularColor * lightColor * specularBRDF;

	return diffuse + specular;
}