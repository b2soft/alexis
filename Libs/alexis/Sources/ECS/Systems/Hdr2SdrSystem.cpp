#include "Precompiled.h"
#include "Hdr2SdrSystem.h"

#include <Core/Core.h>
#include <Core/ResourceManager.h>

#include <Render/Render.h>
#include <Render/CommandContext.h>
#include <Render/RenderTarget.h>

#include <Render/Materials/Hdr2SdrMaterial.h>

namespace alexis
{
	namespace ecs
	{
		void Hdr2SdrSystem::Init()
		{
			m_hdr2SdrMaterial = std::make_unique<Hdr2SdrMaterial>();

			m_fsQuad = Core::Get().GetResourceManager()->GetMesh(L"$FS_QUAD");
		}

		void Hdr2SdrSystem::Render(CommandContext* context)
		{
			auto render = alexis::Render::GetInstance();
			const auto& backbuffer = render->GetBackbufferRT();
			const auto& backTexture = backbuffer.GetTexture(RenderTarget::Slot::Slot0);

			context->TransitionResource(backTexture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

			context->SetRenderTarget(backbuffer);
			context->SetViewport(backbuffer.GetViewport());
		
			m_hdr2SdrMaterial->SetupToRender(context);
		
			m_fsQuad->Draw(context);
		
			context->TransitionResource(backTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
	}
}
