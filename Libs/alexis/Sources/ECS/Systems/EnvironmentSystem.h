#pragma once

#include <ECS/ECS.h>

#include <Render/Buffers/GpuBuffer.h>
#include <Render/Materials/EnvRectToCubeMaterial.h>
#include <Render/Materials/SkyboxMaterial.h>

namespace alexis
{
	class CommandContext;
	class Mesh;

	namespace ecs
	{
		class EnvironmentSystem : public System
		{
		public:
			~EnvironmentSystem();
			void Init();
			void Render(CommandContext* context);
			void RenderSkybox(CommandContext* context);

		private:
			TextureBuffer* m_envMap{ nullptr };
			Mesh* m_cubeMesh{ nullptr };
			TextureBuffer m_cubemap;
			std::unique_ptr<alexis::EnvRectToCubeMaterial> m_envRectToCubeMaterial;
			std::unique_ptr<alexis::SkyboxMaterial> m_skyboxMaterial;
		};

	}
}
