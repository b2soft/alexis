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
	float f = (NdotH * a2 - NdotH) * NdotH + 1.0;
	return a2 / (f * f);
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

// Fresnel (F term)
float3 F_Schlick(float3 f0, float VdotH)
{
	float f = pow(1.0 - VdotH, 5.0);
	return f + f0 * (1.0 - f);
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

	// Roughness remapping
	float roughnessSquared = roughness * roughness;
	float3 diffuseColor = (1.0 - metallic) * baseColor.rgb;
	float reflectance = 0.5;

	float3 specularColor = 0.16 * reflectance * reflectance * (1.0 - metallic) + baseColor * metallic;

	float D = D_GGX(NdotH, roughnessSquared);
	float3 F = F_Schlick(specularColor, VdotH);
	float G = V_SmithGGXCorrelated(NdotL, NdotV, roughnessSquared);

	float3 diffuse = diffuseColor * Fd_Lambert();
	float3 specular = lightColor * F * (D * G * k_pi * NdotL);

	//return G * k_invPi;
	return diffuse;
}