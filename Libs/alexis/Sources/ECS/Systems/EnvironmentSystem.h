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
			void CaptureCubemap(CommandContext* context);
			void ConvoluteCubemap(CommandContext* context);
			void RenderSkybox(CommandContext* context);

		private:
			Mesh* m_cubeMesh{ nullptr };
			TextureBuffer m_cubemap;
			TextureBuffer m_irradianceMap;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 6> m_cubemapRTVs;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 6> m_irradianceRTVs;
			Material* m_envRectToCubeMaterial{ nullptr };
			Material* m_irradianceMaterial{ nullptr };
			Material* m_skyboxMaterial{ nullptr };
			bool m_irradianceCalculated{ false };
		};

	}
}
