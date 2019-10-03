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
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float3 bitangent : BITANGENT;
	float2 uv0 : TEXCOORD;
};

struct VSOutput
{
	float4 normal : NORMAL;
	float2 uv0 : TEXCOORD;
	float4 position : SV_Position;
};

VSOutput main(VSInput input)
{
	VSOutput output;

	output.position = mul(MatCB.mvpMatrix, float4(input.position, 1.0f));
	output.normal = mul(MatCB.mvpMatrix, float4(input.normal, 1.0f));
	output.uv0 = input.uv0;

	return output;
}