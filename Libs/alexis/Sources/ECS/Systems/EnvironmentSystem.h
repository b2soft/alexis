#pragma once

#include <ECS/ECS.h>

#include <Render/Buffers/GpuBuffer.h>

namespace alexis
{
	class CommandContext;
	class Material;
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
			Mesh* m_cubeMesh{ nullptr };
			TextureBuffer m_cubemap;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 6> m_cubemapRTVs;
			Material* m_envRectToCubeMaterial{ nullptr };
			Material* m_skyboxMaterial{ nullptr };
		};

	}
}
