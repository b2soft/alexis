#pragma once

#include <DirectXMath.h>

namespace alexis
{
	class Mesh;
	class IMaterial;

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
			IMaterial* Material;

			DirectX::XMMATRIX ModelMatrix;
			bool IsTransformDirty{ true };
		};
	}
}
