#pragma once

#include <ECS/ECS.h>

#include <Render/Materials/MaterialBase.h>

namespace alexis
{
	class CommandContext;
	class Mesh;

	namespace ecs
	{
		class Hdr2SdrSystem : public System
		{
		public:
			void Init();
			void Render(CommandContext* context);

		private:
			std::unique_ptr<MaterialBase> m_hdr2SdrMaterial;
			Mesh* m_fsQuad{ nullptr };
		};
	}
}
