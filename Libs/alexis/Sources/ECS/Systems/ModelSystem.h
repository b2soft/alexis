#pragma once

#include <DirectXMath.h>

#include <ECS/ECS.h>

namespace alexis
{
	class Mesh;
	class Material;
	class CommandContext;

	namespace ecs
	{
		class ModelSystem : public ecs::System
		{
		public:
			void Init();
			void Update(float dt);
			void XM_CALLCONV Render(CommandContext* context);

			void ZPrepass(CommandContext* context);
			void ForwardPass(CommandContext* context);

		private:
			Material* m_zPrepassMaterial{ nullptr };
			Material* m_forwardPassMaterial{ nullptr };
		};
	}
}