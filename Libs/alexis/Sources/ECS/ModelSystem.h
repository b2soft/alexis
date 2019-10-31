#pragma once

#include <DirectXMath.h>

#include <ECS/ECS.h>
#include <ECS/TransformComponent.h>

namespace alexis
{
	class Mesh;
	class CommandContext;

	namespace ecs
	{
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