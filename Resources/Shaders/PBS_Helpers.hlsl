static const float k_pi = 3.14159265358979323846;
static const float k_invPi = 1.0 / k_pi;

// Specular BRDF
// Normal Distribution Function (D Term)
float D_GGX(float NdotH, float roughness)
{
	float a = NdotH * roughness;
	float k = roughness / (1.0 - NdotH * NdotH + a * a);

	return k * k * k_invPi;
}

// Geometric Shadowing (G Term)
// a2 is roughness squared
float V_SmithGGXCorrelated(float NdotL, float NdotV, float a2)
{
	float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
	float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);

	return 0.5 / (GGXV + GGXL);
}

// Fresnel (F term)
// U == LdotH
float3 F_Schlick(float3 f0, float VdotH)
{
	float f = (1.0 - VdotH, 5.0);
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
	float3 h = normalize(l + v);

	float NdotV = abs(dot(n, v)) + 1e-5f;
	float NdotL = saturate(dot(n, l));
	float NdotH = saturate(dot(n, h));
	float LdotH = saturate(dot(l, h));

	// Roughness remapping
	float a = roughness * roughness;
	float3 diffuseColor = (1.0 - metallic) * baseColor.rgb;
	float reflectance = 0.5;

	float3 f0 = 0.16 * reflectance * reflectance * (1.0 - metallic) * baseColor * metallic;

	float D = D_GGX(NdotH, roughness);
	float3 F = F_Schlick(f0, LdotH);
	float V = V_SmithGGXCorrelated(NdotV, NdotL, roughness);

	// Specular BRDF
	float3 specular = D * F * V * k_invPi;

	// Diffuse BRDF
	float3 diffuse = baseColor * Fd_Lambert();

	//return specular;
	return diffuse;
	//return diffuse + specular;
}