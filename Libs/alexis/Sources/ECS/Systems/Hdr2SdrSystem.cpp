#include "Precompiled.h"
#include "Hdr2SdrSystem.h"

#include <Core/Core.h>
#include <Core/ResourceManager.h>

#include <Render/Render.h>
#include <Render/CommandContext.h>
#include <Render/RenderTarget.h>

namespace alexis
{
	namespace ecs
	{
		void Hdr2SdrSystem::Init()
		{
			auto* resMgr = Core::Get().GetResourceManager();
			m_hdr2SdrMaterial = resMgr->GetMaterial(L"Resources/Materials/system/Hdr2Sdr.material");

			m_fsQuad = resMgr->GetMesh(L"$FS_QUAD");
		}

		void Hdr2SdrSystem::Render(CommandContext* context)
		{
			auto render = alexis::Render::GetInstance();
			const auto& backbuffer = render->GetBackbufferRT();
			const auto& backTexture = backbuffer.GetTexture(RenderTarget::Slot::Slot0);

			auto rtManager = render->GetRTManager();
			auto hdrRT = rtManager->GetRenderTarget(L"HDR");

			context->TransitionResource(backTexture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			context->TransitionResource(hdrRT->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			context->SetRenderTarget(backbuffer);
			context->SetViewport(backbuffer.GetViewport());
		
			m_hdr2SdrMaterial->Set(context);
		
			m_fsQuad->Draw(context);
		
			context->TransitionResource(backTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
	}
}
