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
		class ModelSystem : public ecs::System
		{
		public:
			void Update(float dt);
			void Render(CommandContext* context, DirectX::XMMATRIX view/*, DirectX::XMMATRIX proj*/);
		};

	}
}