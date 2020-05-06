struct DepthParams
{
	matrix ModelMatrix;//TODO remove
	matrix DepthMVP;
};

ConstantBuffer<DepthParams> DepthCB : register(b0);


float4 main( float4 pos : POSITION ) : SV_POSITION
{
	matrix mvpMatrix = mul(DepthCB.DepthMVP, DepthCB.ModelMatrix);
	float4 worldPos = float4(pos.xyz, 1.0f);

	return mul(DepthCB.DepthMVP, worldPos);
}