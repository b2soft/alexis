struct CameraParams
{
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
	float4 normal : NORMAL;
	float2 uv0 : TEXCOORD;
	float4 oPos : Test;
	float4 position : SV_Position;
};

VSOutput main(VSInput input)
{
	VSOutput output;

	matrix mvpMatrix = CameraCB.ViewProjMatrix;// mul(ModelCB.ModelMatrix, CameraCB.ViewProjMatrix);

	output.oPos = mul(ModelCB.ModelMatrix, float4(input.position, 1.0f));
	output.position = mul(mvpMatrix, float4(input.position, 1.0f));
	output.normal = mul(mvpMatrix, float4(input.normal, 1.0f));
	output.uv0 = input.uv0;

	return output;
}