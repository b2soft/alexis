#pragma once

#include <DirectXMath.h>

#include <ECS/ECS.h>

namespace alexis
{
	class Mesh;
	class CommandContext;

	namespace ecs
	{
		struct TransformComponent
		{
			DirectX::XMVECTOR Position;
			DirectX::XMVECTOR Rotation;
		};

		struct ModelComponent
		{
			Mesh* Mesh;
			DirectX::XMMATRIX ModelMatrix;
		};

		class ModelSystem : public ecs::System
		{
		public:
			void Render(CommandContext* context);
		};

	}
}