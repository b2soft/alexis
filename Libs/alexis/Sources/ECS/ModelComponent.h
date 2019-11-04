#pragma once

#include <DirectXMath.h>

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
			DirectX::XMMATRIX ModelMatrix;
			bool IsTransformDirty{ true };
		};
	}
}
