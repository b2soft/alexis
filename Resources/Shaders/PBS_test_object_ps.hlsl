struct PSInput
{
	float2 uv0 : TEXCOORD;
	//float3x3 TBN : TBNBASIS;
	float4 normal : NORMAL;
	float4 worldPos : WORLDPOS;
};

struct PSOutput
{
	float4 gb0 : SV_Target0;
	float4 gb1 : SV_Target1;
	float4 gb2 : SV_Target2;
};

Texture2D BaseColor : register(t0);
Texture2D Normal : register(t1);
Texture2D MetalRoughness : register(t2);

SamplerState AnisotropicSampler : register(s0);

PSOutput main(PSInput input)
{
	float4 texColor = BaseColor.Sample(AnisotropicSampler, input.uv0);
	float4 normalMap = Normal.Sample(AnisotropicSampler, input.uv0);
	float4 metalRoughness = MetalRoughness.Sample(AnisotropicSampler, input.uv0);

	float4 worldPos = float4(input.worldPos.xyz, 1.0);

	float roughness = 0.1;
	[flatten] if (worldPos.x >= 1.5 && worldPos.x < 4.5)
	{
		roughness = 0.3;
	}
	else
		[flatten] if (worldPos.x >= 4.5 && worldPos.x < 7.5)
	{
		roughness = 0.5;
	}
		else
		[flatten] if (worldPos.x >= 7.5 && worldPos.x < 10.5)
	{
		roughness = 0.8;
	}
		else
		[flatten] if (worldPos.x >= 10.5)
	{
		roughness = 1.0;
	}

	float metalness = 0.0;
	[flatten] if (worldPos.z >= 1.5 && worldPos.z < 4.5)
	{
		metalness = 0.3;
	}
	else
		[flatten] if (worldPos.z >= 4.5 && worldPos.z < 7.5)
	{
		metalness = 0.5;
	}
		else
		[flatten] if (worldPos.z >= 7.5 && worldPos.z < 10.5)
	{
		metalness = 0.8;
	}
		else
		[flatten] if (worldPos.z >= 10.5)
	{
		metalness = 1.0;
	}

	metalRoughness.r = metalness;
	metalRoughness.g = roughness;

	float4 normal = float4(normalize(input.normal.xyz), 1.0);
	normal.xyz = normal.xyz * 0.5f + 0.5f;

	PSOutput output;
	//output.gb0 = texColor;
	output.gb0 = float4(1.0, 0.0, 0.0, 1.0);
	output.gb1 = normal;
	output.gb2 = metalRoughness;

	return output;
}