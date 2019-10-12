struct PSInput
{
	float2 uv : TEXCOORD0;
	float4 position : SV_POSITION;
};


PSInput main(float4 position : POSITION, float4 uv : TEXCOORD)
{
	PSInput result;

	result.position = position;
	result.uv = uv;

	return result;
}