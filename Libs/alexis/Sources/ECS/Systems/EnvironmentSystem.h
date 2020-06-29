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
			void CapturePreFilteredTexture(CommandContext* context);
			void ConvoluteBRDF(CommandContext* context);
			void RenderSkybox(CommandContext* context);

		private:
			Mesh* m_cubeMesh{ nullptr };
			TextureBuffer m_cubemap;
			TextureBuffer m_irradianceMap;
			TextureBuffer m_prefilteredMap;
			TextureBuffer m_convolutedBRDFMap;
			//TODO: Create CubeRT and RT encapsulating RTVs
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 6> m_cubemapRTVs;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 6> m_irradianceRTVs;

			static constexpr std::size_t k_munMipLevels = 6;
			std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 6 * k_munMipLevels + k_munMipLevels> m_prefilteredRTVs;

			Material* m_envRectToCubeMaterial{ nullptr };
			Material* m_irradianceMaterial{ nullptr };
			Material* m_skyboxMaterial{ nullptr };
			Material* m_prefilteredMaterial{ nullptr };
			Material* m_convoluteBRDFMaterial{ nullptr };
			Mesh* m_fsQuad{ nullptr };

			bool m_irradianceCalculated{ false };
		};

	}
}
