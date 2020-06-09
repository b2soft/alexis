#include "Precompiled.h"

#include "EnvironmentSystem.h"

#include <Core/Core.h>
#include <Core/ResourceManager.h>
#include <Render/Render.h>
#include <Render/CommandContext.h>
#include <Render/RenderTargetManager.h>

#include <ECS/Systems/CameraSystem.h>
#include <ECS/Components/TransformComponent.h>

namespace alexis
{

	__declspec(align(16)) struct CameraParams
	{
		XMMATRIX ViewMatrix;
		XMMATRIX ProjMatrix;
	};

	ecs::EnvironmentSystem::~EnvironmentSystem()
	{

	}

	void ecs::EnvironmentSystem::Init()
	{
		auto* rm = Core::Get().GetResourceManager();
		m_cubeMesh = rm->GetMesh(L"Resources/Models/Cube.DAE");
		m_envMap = rm->GetTexture(L"Resources/Textures/rooitou_park_2k.hdr");

		m_envRectToCubeMaterial = std::make_unique<EnvRectToCubeMaterial>();
		m_skyboxMaterial = std::make_unique<SkyboxMaterial>();

		D3D12_RESOURCE_DESC desc;
		desc.Width = 512;
		desc.Height = 512;
		desc.MipLevels = 1;
		desc.Alignment = 0;
		desc.DepthOrArraySize = 6;
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		m_cubemap.Create(desc);
		m_cubemap.GetResource()->SetName(L"CUBEMAP");
	}

	void ecs::EnvironmentSystem::Render(CommandContext* context)
	{
		auto& ecsWorld = Core::Get().GetECSWorld();
		auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
		auto activeCamera = cameraSystem->GetActiveCamera();

		auto* render = alexis::Render::GetInstance();
		auto* rtManager = render->GetRTManager();


		XMMATRIX projMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(90.0f), 1.0f, 0.1f, 10.f);
		std::array<XMMATRIX, 6> viewMatrices =
		{
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }),
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }),
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }),
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }),
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }),
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f }),
		};

		context->TransitionResource(m_cubemap, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

		for (int i = 0; i < viewMatrices.size(); ++i)
		{
			m_envRectToCubeMaterial->SetupToRender(context);

			D3D12_CPU_DESCRIPTOR_HANDLE rtv[] = { m_cubemap.GetRTV(i) };
			const float clearColor[4] = { 0, 0, 0, 0 };
			context->List->ClearRenderTargetView(m_cubemap.GetRTV(i), clearColor, 0, nullptr);
			context->List->OMSetRenderTargets(1, rtv, FALSE, nullptr);

			//context->SetRenderTarget(*hdrRt);
			D3D12_VIEWPORT viewport{ 0, 0, 512, 512 };

			context->SetViewport(Viewport{ viewport, Render::GetInstance()->GetDefaultScissorRect() });

			CameraParams cameraParams;
			cameraParams.ViewMatrix = viewMatrices[i];
			cameraParams.ProjMatrix = projMatrix;
			context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);
			context->SetSRV(1, 0, *m_envMap);

			m_cubeMesh->Draw(context);
		}
	}

	void ecs::EnvironmentSystem::RenderSkybox(CommandContext* context)
	{
		auto& ecsWorld = Core::Get().GetECSWorld();
		auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
		auto activeCamera = cameraSystem->GetActiveCamera();

		auto* render = alexis::Render::GetInstance();
		auto* rtManager = render->GetRTManager();

		m_skyboxMaterial->SetupToRender(context);

		auto* rt = rtManager->GetRenderTarget(L"HDR");
		context->SetRenderTarget(*rt);
		context->SetViewport(rt->GetViewport());

		context->TransitionResource(m_cubemap, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		context->TransitionResource(rt->GetTexture(RenderTarget::Slot0), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

		CameraParams cameraParams;
		cameraParams.ViewMatrix = cameraSystem->GetViewMatrix(activeCamera);
		cameraParams.ProjMatrix = cameraSystem->GetProjMatrix(activeCamera);
		context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);
		context->SetSRV(1, 0, m_cubemap);

		m_cubeMesh->Draw(context);
	}

}
