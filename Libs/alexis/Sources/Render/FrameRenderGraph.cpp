#include "Precompiled.h"

#include "FrameRenderGraph.h"

#include <ECS/ECS.h>
#include <Core/Core.h>
#include <Render/Render.h>

#include <ECS/ModelSystem.h>
#include <ECS/CameraSystem.h>
#include <ECS/ShadowSystem.h>
#include <ECS/LightingSystem.h>
#include <ECS/Hdr2SdrSystem.h>
#include <ECS/ImguiSystem.h>

namespace alexis
{

	void FrameRenderGraph::Render()
	{
		auto ecsWorld = Core::Get().GetECS();
		auto render = alexis::Render::GetInstance();
		auto commandManager = render->GetCommandManager();

		auto gbuffer = render->GetRTManager()->GetRenderTarget(L"GB");
		auto hdrRT = render->GetRTManager()->GetRenderTarget(L"HDR");
		auto shadowRT = render->GetRTManager()->GetRenderTarget(L"Shadow Map");

		// TODO: RTManager flush every frame flag impl
		auto clearTargetContext = commandManager->CreateCommandContext();
		auto clearTask = [gbuffer, hdrRT, shadowRT, clearTargetContext]()
		{
			static constexpr float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

			// Clear G-Buffer
			auto& gbDepthStencil = gbuffer->GetTexture(RenderTarget::DepthStencil);

			for (int i = RenderTarget::Slot::Slot0; i < RenderTarget::Slot::DepthStencil; ++i)
			{
				auto& texture = gbuffer->GetTexture(static_cast<RenderTarget::Slot>(i));
				if (texture.IsValid())
				{
					clearTargetContext->TransitionResource(texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
					clearTargetContext->ClearTexture(texture, clearColor);
				}
			}

			clearTargetContext->TransitionResource(gbDepthStencil, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			clearTargetContext->ClearDepthStencil(gbDepthStencil, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0);

			// Clear HDR
			auto& texture = hdrRT->GetTexture(static_cast<RenderTarget::Slot>(RenderTarget::Slot::Slot0));
			clearTargetContext->TransitionResource(texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
			clearTargetContext->ClearTexture(texture, clearColor);

			{
				auto& shadowDepth = shadowRT->GetTexture(RenderTarget::DepthStencil);
				clearTargetContext->TransitionResource(shadowDepth, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
				clearTargetContext->ClearDepthStencil(shadowDepth, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0);
			}
		};
		clearTask();

		// PBR models rendering
		auto pbsContext = commandManager->CreateCommandContext();
		auto pbrTask = [pbsContext, gbuffer, ecsWorld]
		{
			auto modelSystem = ecsWorld->GetSystem<ecs::ModelSystem>();
			modelSystem->Render(pbsContext);
		};
		pbrTask();

		// Shadows Cast
		auto shadowContext = commandManager->CreateCommandContext();
		auto shadowTask = [shadowContext, ecsWorld]
		{
			auto shadowSystem = ecsWorld->GetSystem<ecs::ShadowSystem>();
			shadowSystem->Render(shadowContext);
		};
		shadowTask();

		// Lighting Resolve
		auto lightingContext = commandManager->CreateCommandContext();
		auto lightingTask = [lightingContext, ecsWorld]
		{
			auto lightingSystem = ecsWorld->GetSystem<ecs::LightingSystem>();
			lightingSystem->Render(lightingContext);
		};
		lightingTask();

		// HDR resolve
		auto hdrContext = commandManager->CreateCommandContext();
		auto hdrTask = [hdrContext, ecsWorld]
		{
			auto hdr2SdrSystem = ecsWorld->GetSystem<ecs::Hdr2SdrSystem>();
			hdr2SdrSystem->Render(hdrContext);
		};
		hdrTask();

		// ImGUI
		auto imguiContext = commandManager->CreateCommandContext();
		auto imguiTask = [imguiContext, ecsWorld]
		{
			auto imguiSystem = ecsWorld->GetSystem<ecs::ImguiSystem>();
			imguiSystem->Render(imguiContext);
		};
		imguiTask();

		// Flush
		{
			clearTargetContext->Finish();
			pbsContext->Finish();
			shadowContext->Finish();
			lightingContext->Finish();
			hdrContext->Finish();
			imguiContext->Finish();
		}
	}
}

