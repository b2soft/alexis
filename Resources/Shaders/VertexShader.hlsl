struct PSInput
{
	float4 color : COLOR;
	float4 position : SV_POSITION;
};


PSInput main(float4 position : POSITION, float4 color : COLOR)
{
	PSInput result;

	result.position = position;
	result.color = color;

	return result;
}