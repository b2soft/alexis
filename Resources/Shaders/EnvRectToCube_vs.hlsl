struct CameraParams
{
	matrix ViewMatrix;
	matrix ProjMatrix;
};

struct VSOutput
{
	float4 localPos : POSITION;
	float4 position : SV_Position;
};

ConstantBuffer<CameraParams> CamCB : register(b0);

VSOutput main(float4 pos : POSITION)
{
	VSOutput output;
	output.position = mul(CamCB.ProjMatrix, mul(CamCB.ViewMatrix, pos));
	output.localPos = pos;

	return output;
}