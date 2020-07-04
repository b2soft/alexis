#define RootSig "RootFlags(0)," \
"CBV(b0, space = 0, flags = DATA_STATIC)," \
"CBV(b1, space = 0, flags = DATA_STATIC)," \
"DescriptorTable(UAV(u0))"

struct ClusterAABB
{
	float4 MinPoint;
	float4 MaxPoint;
};

struct ScreenToViewParams
{
	matrix InvProj;
	uint4 ClusterSize;
	uint2 ScreenSize;
};

struct CameraParams
{
	float Near;
	float Far;
};

ConstantBuffer<ScreenToViewParams> ScreenToViewCB : register(b0);
ConstantBuffer<CameraParams> CameraCB : register(b1);

RWStructuredBuffer<ClusterAABB> ClustersUAV : register(u0);

struct ComputeShaderInput
{
	uint3 GroupID : SV_GroupID;
	uint3 GroupThreadID : SV_GroupThreadID;
	uint3 DisptatchThreadID : SV_DispatchThreadID;
	uint GroupIndex : SV_GroupIndex;
};

float4 ClipToView(float4 clipPos)
{
	float4 viewPos = mul(ScreenToViewCB.InvProj, clipPos);

	viewPos = viewPos / viewPos.w;

	return viewPos;
}

float4 ScreenToView(float4 screenPos)
{
	// To NDC
	float2 texCoord = screenPos.xy / ScreenToViewCB.ScreenSize;

	// To Clip space
	float4 clipPos = float4(float2(texCoord.x, texCoord.y) * 2.0 - 1.0, screenPos.z, screenPos.w);

	return ClipToView(clipPos);
}

float3 LineIntersectionToZPlane(float3 A, float3 B, float zDistance)
{
	float3 normal = float3(0.0, 0.0, 1.0);

	float3 ab = A - B;

	float t = (zDistance - dot(normal, A)) / max(dot(normal, ab), 0.0001);

	float3 result = A = t * ab;

	return result;
}

static const uint3 numGroups = uint3(16, 9, 24);

[RootSignature(RootSig)]
[numthreads(1, 1, 1)]
void main(ComputeShaderInput input)
{
	// Camera position is zero in View Space
	float3 camPos = float3(0.0, 0.0, 0.0);

	// Per Cluster variables
	uint clusterSizePx = ScreenToViewCB.ClusterSize[3];
	uint clusterIndex = input.GroupID.x + input.GroupID.y * numGroups.x + input.GroupID.z * (numGroups.x * numGroups.y);

	// Calculate min and max in screen space
	float4 maxPointScreenSpace = float4(float2(input.GroupID.x + 1, input.GroupID.y + 1) * clusterSizePx, 1.0, 1.0); // Top Right
	float4 minPointScreenSpace = float4(input.GroupID.xy * clusterSizePx, 1.0, 1.0); // Bottom Left

	// Pass min and max valueso to view space
	float3 maxPointViewSpace = ScreenToView(maxPointScreenSpace).xyz;
	float3 minPointViewSpace = ScreenToView(minPointScreenSpace).xyz;

	// Cluster Near and Far values in View Space
	float clusterNear = -CameraCB.Near * pow(CameraCB.Far / CameraCB.Near, input.GroupID.z / float(numGroups.z));
	float clusterFar = -CameraCB.Near * pow(CameraCB.Far / CameraCB.Near, (input.GroupID.z + 1) / float(numGroups.z));

	// Find 4 intersection points from maxPoint and cluster near/far plane
	float3 minPointNear = LineIntersectionToZPlane(camPos, minPointViewSpace, clusterNear);
	float3 minPointFar = LineIntersectionToZPlane(camPos, minPointViewSpace, clusterFar);
	float3 maxPointNear = LineIntersectionToZPlane(camPos, maxPointViewSpace, clusterNear);
	float3 maxPointFar = LineIntersectionToZPlane(camPos, maxPointViewSpace, clusterFar);

	float3 minPointAABB = min(min(minPointNear, minPointFar), min(maxPointNear, maxPointFar));
	float3 maxPointAABB = max(max(minPointNear, minPointFar), max(maxPointNear, maxPointFar));

	ClustersUAV[0].MinPoint = float4(1.0, 2.0, 3.0, 4.0);
	ClustersUAV[0].MaxPoint = float4(maxPointAABB, 0.0);
}