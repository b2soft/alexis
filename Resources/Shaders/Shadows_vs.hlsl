struct DepthParams
{
	matrix DepthMVP;
};

ConstantBuffer<DepthParams> DepthCB : register(b0);


float4 main( float4 pos : POSITION ) : SV_POSITION
{
	return mul(pos, DepthCB.DepthMVP);
}