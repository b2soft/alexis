struct PSOutput
{
	float4 gb0 : SV_Target0;
	float4 gb1 : SV_Target1;
	float4 gb2 : SV_Target2;
};

PSOutput main(float4 Pos : SV_Position)
{
	PSOutput output;
	output.gb0 = float4(1.0, 1.0, 1.0, 1.0);
	output.gb1 = float4(0.5, 0.5, 1.0, 1.0);
	output.gb2 = float4(0, 0.5, 1.0, 0.0);

	return output;
}