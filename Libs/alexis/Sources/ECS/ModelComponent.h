#pragma once

#include <DirectXMath.h>

#include <Render/Buffers/GpuBuffer.h>

namespace alexis
{
	class Mesh;

	namespace ecs
	{
		struct ModelComponent
		{
			struct Mat
			{
				DirectX::XMMATRIX ModelMatrix;
				DirectX::XMMATRIX ModelViewMatrix;
				DirectX::XMMATRIX ModelViewProjectionMatrix;
			};

			Mesh* Mesh;
			alexis::DynamicConstantBuffer CBV;
			DirectX::XMMATRIX ModelMatrix;
			bool IsTransformDirty{ true };
		};
	}
}
