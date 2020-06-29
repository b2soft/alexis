#pragma once

#include <DirectXMath.h>

namespace alexis
{
	class Mesh;
	class Material;

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
			Material* Material;

			DirectX::XMMATRIX ModelMatrix;
			bool IsTransformDirty{ true };
		};
	}
}
