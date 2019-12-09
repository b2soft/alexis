struct CameraParams
{
	matrix ViewMatrix;
	matrix ViewProjMatrix;
};

struct ModelParams
{
	matrix ModelMatrix;
};

ConstantBuffer<CameraParams> CameraCB : register(b0);
ConstantBuffer<ModelParams> ModelCB : register(b1);

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
	float2 uv0 : TEXCOORD;
	float4 normal : NORMAL;
	float4 position : SV_Position;
};

VSOutput main(VSInput input)
{
	VSOutput output;

	matrix mvpMatrix = mul(CameraCB.ViewProjMatrix, ModelCB.ModelMatrix);
	float4 worldPos = float4(input.position, 1.0f);

	output.position = mul(mvpMatrix, worldPos);
	output.uv0 = input.uv0;

	float4 normal = mul(ModelCB.ModelMatrix, float4(input.normal, 0.0f));
	output.normal = normal;

	return output;
}