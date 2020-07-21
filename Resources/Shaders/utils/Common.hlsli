float3 GetWorldPosFromDepth(float depth, float2 screenPos, matrix invView, matrix invProj)
{
	float z = depth;
	float x = screenPos.x * 2 - 1;
	float y = (1 - screenPos.y) * 2 - 1;

	float4 clipSpacePosition = float4(x, y, z, 1.0f);
	float4 viewSpacePosition = mul(invProj, clipSpacePosition);

	// Perspective division
	viewSpacePosition /= viewSpacePosition.w;

	float4 worldSpacePosition = mul(invView, viewSpacePosition);

	return worldSpacePosition.xyz;
}