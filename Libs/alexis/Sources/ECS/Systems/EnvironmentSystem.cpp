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

		D3D12_RESOURCE_DESC desc{};
		desc.Width = 1024;
		desc.Height = 1024;
		desc.MipLevels = 1;
		desc.Alignment = 0;
		desc.DepthOrArraySize = 6;
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		m_cubemap.Create(desc);

		auto* rtManager = Render::GetInstance()->GetRTManager();
		auto cubemapRT = std::make_unique<RenderTarget>();
		cubemapRT->AttachTexture(m_cubemap, RenderTarget::Slot0);
		rtManager->EmplaceTarget(L"CUBEMAP", std::move(cubemapRT));

		for (int i = 0; i < 6; ++i)
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Format = desc.Format;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.PlaneSlice = 0;

			rtvDesc.Texture2DArray.FirstArraySlice = i;
			rtvDesc.Texture2DArray.ArraySize = 1;

			auto alloc = Render::GetInstance()->AllocateRTV(m_cubemap.GetResource(), rtvDesc);
			m_cubemapRTVs[i] = alloc.CpuPtr;
		}

		m_envRectToCubeMaterial = rm->GetBetterMaterial(L"Resources/Materials/system/EnvRectToCube.material");
		m_skyboxMaterial = rm->GetBetterMaterial(L"Resources/Materials/system/Skybox.material");
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
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }),		// +X
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }),	// -X
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }),	// +Y
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }),	// -Y
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }),		// +Z
			XMMatrixLookAtLH({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f }),	// -Z
		};

		context->TransitionResource(m_cubemap, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

		for (int i = 0; i < viewMatrices.size(); ++i)
		{
			m_envRectToCubeMaterial->Set(context);

			//const float clearColor[4] = { 0, 0, 0, 0 };
			//context->List->ClearRenderTargetView(m_cubemapRTVs[i], clearColor, 0, nullptr);
			context->List->OMSetRenderTargets(1, &m_cubemapRTVs[i], FALSE, nullptr);

			D3D12_VIEWPORT viewport{ 0, 0, 1024, 1024 };
			CD3DX12_RECT rect{ 0, 0,1024, 1024 };

			context->SetViewport(Viewport{ viewport, rect });

			CameraParams cameraParams;
			cameraParams.ViewMatrix = viewMatrices[i];
			cameraParams.ProjMatrix = projMatrix;
			context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);

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

		context->TransitionResource(m_cubemap, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_skyboxMaterial->Set(context);

		auto* rt = rtManager->GetRenderTarget(L"HDR");
		context->SetRenderTarget(*rt);
		context->SetViewport(rt->GetViewport());

		context->TransitionResource(rt->GetTexture(RenderTarget::Slot0), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
		context->TransitionResource(rt->GetTexture(RenderTarget::DepthStencil), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_READ);

		CameraParams cameraParams;
		cameraParams.ViewMatrix = cameraSystem->GetViewMatrix(activeCamera);
		cameraParams.ProjMatrix = cameraSystem->GetProjMatrix(activeCamera);
		context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);

		m_cubeMesh->Draw(context);
	}

}
