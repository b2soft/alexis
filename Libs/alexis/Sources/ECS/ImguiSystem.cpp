#include "Precompiled.h"

#include "ImguiSystem.h"

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include <Core/Core.h>
#include <Render/Render.h>
#include <Render/RenderTarget.h>

namespace alexis
{
	namespace ecs
	{
		ImguiSystem::~ImguiSystem()
		{
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext(m_context);
		}

		void ImguiSystem::Init()
		{
			// Create Descriptor Heaps
			{
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				desc.NumDescriptors = 1;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				ThrowIfFailed(Render::GetInstance()->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_imguiSrvHeap)));
			}

			IMGUI_CHECKVERSION();
			m_context = ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();

			ImGui::StyleColorsLight();

			ImGui_ImplWin32_Init(Core::GetHwnd());
			ImGui_ImplDX12_Init(Render::GetInstance()->GetDevice(), 2,
				DXGI_FORMAT_R8G8B8A8_UNORM,
				m_imguiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
				m_imguiSrvHeap->GetGPUDescriptorHandleForHeapStart());
		}

		void ImguiSystem::Update(float dt)
		{
			ImGui::SetCurrentContext(m_context);
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
		}

		void ImguiSystem::Render(CommandContext* context)
		{
			auto render = Render::GetInstance();
			const auto& backbuffer = render->GetBackbufferRT();
			const auto& backTexture = backbuffer.GetTexture(RenderTarget::Slot::Slot0);

			context->TransitionResource(backTexture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

			context->SetRenderTarget(backbuffer);
			context->SetViewport(backbuffer.GetViewport());

			ID3D12DescriptorHeap* imguiHeap[] = { m_imguiSrvHeap.Get() };
			context->List->SetDescriptorHeaps(_countof(imguiHeap), imguiHeap);
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), context->List.Get());

			context->TransitionResource(backTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
	}
}