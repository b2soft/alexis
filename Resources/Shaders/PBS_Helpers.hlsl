static const float k_pi = 3.14159265358979323846;
static const float k_invPi = 1.0 / k_pi;

// Specular BRDF
// Normal Distribution Function (D Term)
float D_GGX(float NoH, float roughness)
{
	float a = NoH * roughness;
	float k = roughness / (1.0 - NoH * NoH + a * a);
	return k * k * k_invPi;
}

// Geometric Shadowing (G Term)
float V_SmithGGXCorrelated(float NoV, float NoL, float roughness)
{
	float a2 = roughness * roughness;
	float GGXV = NoL * sqrt(NoV * NoV * (1.0 - a2) + a2);
	float GGXL = NoV * sqrt(NoL * NoL * (1.0 - a2) + a2);

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
//float3 F_Schlick(float VoH, float3 f0, float f90)
//{
//	return f0 + (float3(f90, f90, f90) - f0) * pow(1.0 - VoH, 5.0);
//}

// Considering f90 = 1.0
float3 F_Schlick(float VoH, float3 f0)
{
	float f = pow(1.0 - VoH, 5.0);

	return f + f0 * (1.0 - f);
}

// Diffuse BRDF
// Lambertian BRDF
float Fd_Lambert()
{
	return k_invPi;
}

float3 BRDF(float3 v, float3 l, float3 n, float perceptualRoughness, float f0, float3 diffuseColor)
{
	float3 h = normalize(v + l);

	float NdotV = abs(dot(n, v)) + 1e-5;
	float NdotL = clamp(dot(n, l), 0.0, 1.0);
	float NdotH = clamp(dot(n, h), 0.0, 1.0);
	float LdotH = clamp(dot(l, h), 0.0, 1.0);

	float roughness = perceptualRoughness * perceptualRoughness;

	float D = D_GGX(NdotH, roughness);
	float3 F = F_Schlick(LdotH, f0);
	float V = V_SmithGGXCorrelated(NdotV, NdotL, roughness);

	// Specular BRDF
	float3 specular = (D * V) * F;

	// Diffuse BRDF
	float3 diffuse = diffuseColor * Fd_Lambert();

	return diffuse + specular;
}