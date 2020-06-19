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
			Material* m_hdr2SdrMaterial{ nullptr };
			Mesh* m_fsQuad{ nullptr };
		};
	}
}
