struct Mat
{
	matrix mMatrix;
	matrix mvMatrix;
	matrix mvpMatrix;
};

ConstantBuffer<Mat> MatCB : register(b0);

struct VSInput
{
	float3 position : POSITION;
};


struct VSOutput
{
	float4 color: COLOR;
	float4 position : SV_Position;
};

VSOutput main(VSInput input)
{
	VSOutput output;

	output.position = mul(MatCB.mvpMatrix, float4(input.position, 1.0f));
	output.color = float4(1.0f, 0.0f, 1.0f, 1.0f);

	return output;
}