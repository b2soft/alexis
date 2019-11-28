static const float k_pi = 3.14159265358979323846;
static const float k_invPi = 1.0 / k_pi;

// Specular BRDF
// Normal Distribution Function (D Term)
float D_GGX(float NoH, float roughness)
{
	float f = (NoH * roughness - NoH) * NoH + 1;
	return roughness / (f * f);
}

// Geometric Shadowing (G Term)
float G_SmithGGXCorrelated(float NoV, float NoL, float roughness)
{
	float GGXV = NoL * sqrt((-NoV * roughness + NoV) * NoV + roughness);
	float GGXL = NoV * sqrt((-NoL * roughness + NoL) * NoL + roughness);

	return 0.5 / (GGXV + GGXL);
}

// Physically wrong but not using square roots
float V_SmithGGXCorrelatedFast(float NoV, float NoL, float roughness)
{
	float a = roughness;
	float GGXV = NoL * (NoV * (1.0 - a) + a);
	float GGXL = NoV * (NoL * (1.0 - a) + a);

	return 0.5 / (GGXV + GGXL);
}

// Fresnel (F term)
float3 F_Schlick(float LoH, float3 f0, float f90)
{
	return f0 + (f90 - f0) * pow(1.0 - LoH, 5.0);
}

// Considering f90 = 1.0
//float3 F_Schlick(float VoH, float3 f0)
//{
//	float f = pow(1.0 - VoH, 5.0);
//
//	return f0 + f0 * (1.0 - f);
//}

// Diffuse BRDF
// Lambertian BRDF
float Fd_Lambert()
{
	return k_invPi;
}

float3 BRDF(float3 v, float3 l, float3 n, float perceptualRoughness, float f0, float3 diffuseColor)
{
	float3 h = normalize(l + v);

	float NdotV = abs(dot(n, v)) + 1e-5;
	float NdotL = saturate(dot(n, l));
	float NdotH = saturate(dot(n, h));
	float LdotH = saturate(dot(l, h));

	float roughness = perceptualRoughness * perceptualRoughness;

	float D = D_GGX(NdotH, roughness);
	float3 F = F_Schlick(LdotH, f0, 1.0);
	float G = G_SmithGGXCorrelated(NdotV, NdotL, roughness);

	// Specular BRDF
	float3 specular = float3(1.0, 1.0, 1.0) * F * (D * G * k_pi * NdotL);


	// Diffuse BRDF
	float3 diffuse = diffuseColor * Fd_Lambert();

	return specular;
	//return diffuse;
	return diffuse + specular;
}