#pragma once

#include <ECS/ECS.h>
#include <optional>

namespace alexis
{
	class Material;
	class CommandContext;
	class Mesh;

	namespace ecs
	{
		class LightingSystem : public System
		{
		public:
			void Init();
			void Render(CommandContext* context);
			void BuildClusters(CommandContext* context);
			void ReadClusters();

		private:
			Material* m_lightingMaterial{ nullptr };
			Mesh* m_fsQuad{ nullptr };

			Microsoft::WRL::ComPtr<ID3D12Resource> m_uavResource;
			Microsoft::WRL::ComPtr<ID3D12Resource> m_uavReadback;
			std::optional<std::size_t> m_uavOffset;
			CD3DX12_CPU_DESCRIPTOR_HANDLE m_uavHandle;
			Material* m_buildClustersMaterial{ nullptr };
		};
	}
}