#pragma once

#include <d3dx12.h>
#include <ECS/ECS.h>

struct ImGuiContext;

namespace alexis
{
	class CommandContext;
	class Mesh;

	namespace ecs
	{
		class ImguiSystem : public System
		{
		public:
			~ImguiSystem();

			void Init();
			void Update(float dt);
			void Render(CommandContext* context);

			Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_imguiSrvHeap;
			ImGuiContext* m_context{ nullptr };
		};
	}
}