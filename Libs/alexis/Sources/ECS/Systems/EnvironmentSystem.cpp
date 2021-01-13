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

	namespace
	{
		static constexpr UINT k_irradianceMapSize = 32;
		static constexpr UINT k_cubemapSize = 1024;
	}

	__declspec(align(16)) struct CameraParams
	{
		XMMATRIX ViewMatrix;
		XMMATRIX ProjMatrix;
	};

	__declspec(align(16)) struct RoughnessParams
	{
		float Roughness;
	};

	ecs::EnvironmentSystem::~EnvironmentSystem()
	{

	}

	void ecs::EnvironmentSystem::Init()
	{
		auto* rm = Core::Get().GetResourceManager();
		m_cubeMesh = rm->GetMesh(L"Resources/Models/Cube.DAE");

		D3D12_RESOURCE_DESC desc{};
		desc.Width = k_cubemapSize;
		desc.Height = k_cubemapSize;
		desc.MipLevels = 1;
		desc.Alignment = 0;
		desc.DepthOrArraySize = 6;
		desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		m_cubemap.Create(desc);
		m_cubemap.GetResource()->SetName(L"Env Cubemap");

		desc.Width = k_irradianceMapSize;
		desc.Height = k_irradianceMapSize;
		m_irradianceMap.Create(desc);
		m_irradianceMap.GetResource()->SetName(L"Irradiance Map");

		desc.Width = 512;
		desc.Height = 512;
		desc.MipLevels = 1;
		m_convolutedBRDFMap.Create(desc);
		m_convolutedBRDFMap.GetResource()->SetName(L"Convoluted BRDF Map");

		desc.Width = 128;
		desc.Height = 128;
		desc.MipLevels = 6;
		m_prefilteredMap.Create(desc);
		m_prefilteredMap.GetResource()->SetName(L"Prefiltered Map");

		auto* rtManager = Render::GetInstance()->GetRTManager();
		auto cubemapRT = std::make_unique<RenderTarget>();
		cubemapRT->AttachTexture(m_cubemap, RenderTarget::Slot0);
		rtManager->EmplaceTarget(L"CUBEMAP", std::move(cubemapRT));

		auto irradianceRT = std::make_unique<RenderTarget>();
		irradianceRT->AttachTexture(m_irradianceMap, RenderTarget::Slot0);
		rtManager->EmplaceTarget(L"CUBEMAP_Irradiance", std::move(irradianceRT));

		auto prefilteredRT = std::make_unique<RenderTarget>();
		prefilteredRT->AttachTexture(m_prefilteredMap, RenderTarget::Slot0);
		rtManager->EmplaceTarget(L"CUBEMAP_Prefiltered", std::move(prefilteredRT));

		auto convolutedBRDFRT = std::make_unique<RenderTarget>();
		convolutedBRDFRT->AttachTexture(m_convolutedBRDFMap, RenderTarget::Slot0);
		rtManager->EmplaceTarget(L"ConvolutedBRDF", std::move(convolutedBRDFRT));

		// Env cubemap
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

		// Irradiance map
		for (int i = 0; i < 6; ++i)
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Format = desc.Format;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.PlaneSlice = 0;

			rtvDesc.Texture2DArray.FirstArraySlice = i;
			rtvDesc.Texture2DArray.ArraySize = 1;

			auto alloc = Render::GetInstance()->AllocateRTV(m_irradianceMap.GetResource(), rtvDesc);
			m_irradianceRTVs[i] = alloc.CpuPtr;
		}

		// Prefiltered map
		for (int i = 0; i < 6; ++i)
		{
			for (int mip = 0; mip < k_munMipLevels; ++mip)
			{
				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				rtvDesc.Format = desc.Format;
				rtvDesc.Texture2DArray.MipSlice = mip;
				rtvDesc.Texture2DArray.PlaneSlice = 0;

				rtvDesc.Texture2DArray.FirstArraySlice = i;
				rtvDesc.Texture2DArray.ArraySize = 1;

				auto alloc = Render::GetInstance()->AllocateRTV(m_prefilteredMap.GetResource(), rtvDesc);
				m_prefilteredRTVs[i * k_munMipLevels + mip] = alloc.CpuPtr;
			}
		}

		m_envRectToCubeMaterial = rm->GetMaterial(L"Resources/Materials/system/EnvRectToCube.material");
		m_skyboxMaterial = rm->GetMaterial(L"Resources/Materials/system/Skybox.material");
		m_irradianceMaterial = rm->GetMaterial(L"Resources/Materials/system/IrradianceMap.material");
		m_prefilteredMaterial = rm->GetMaterial(L"Resources/Materials/system/PrefilteredMap.material");
		m_convoluteBRDFMaterial = rm->GetMaterial(L"Resources/Materials/system/ConvoluteBRDF.material");

		m_fsQuad = Core::Get().GetResourceManager()->GetMesh(L"$FS_QUAD");
	}

	void ecs::EnvironmentSystem::CaptureCubemap(CommandContext* context)
	{
		PIXScopedEvent(context->List.Get(), PIX_COLOR(0, 255, 0), "EnvSystem: Capture Cubemap");

		if (m_irradianceCalculated)
		{
			return;
		}

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

			D3D12_VIEWPORT viewport{ 0, 0, k_cubemapSize, k_cubemapSize };
			CD3DX12_RECT rect{ 0, 0, k_cubemapSize, k_cubemapSize };

			context->SetViewport(Viewport{ viewport, rect });

			CameraParams cameraParams;
			cameraParams.ViewMatrix = viewMatrices[i];
			cameraParams.ProjMatrix = projMatrix;
			context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);

			m_cubeMesh->Draw(context);
		}
	}

	void ecs::EnvironmentSystem::ConvoluteCubemap(CommandContext* context)
	{
		PIXScopedEvent(context->List.Get(), PIX_COLOR(0, 255, 0), "EnvSystem: Convolute Cubemap");

		if (m_irradianceCalculated)
		{
			return;
		}

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

		context->TransitionResource(m_irradianceMap, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
		context->TransitionResource(m_cubemap, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		for (int i = 0; i < viewMatrices.size(); ++i)
		{
			m_irradianceMaterial->Set(context);

			//const float clearColor[4] = { 0, 0, 0, 0 };
			//context->List->ClearRenderTargetView(m_cubemapRTVs[i], clearColor, 0, nullptr);
			context->List->OMSetRenderTargets(1, &m_irradianceRTVs[i], FALSE, nullptr);

			D3D12_VIEWPORT viewport{ 0, 0, k_irradianceMapSize, k_irradianceMapSize };
			CD3DX12_RECT rect{ 0, 0, k_irradianceMapSize, k_irradianceMapSize };

			context->SetViewport(Viewport{ viewport, rect });

			CameraParams cameraParams;
			cameraParams.ViewMatrix = viewMatrices[i];
			cameraParams.ProjMatrix = projMatrix;
			context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);

			m_cubeMesh->Draw(context);
		}

		m_irradianceCalculated = true;

		context->TransitionResource(m_irradianceMap, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
	}

	void ecs::EnvironmentSystem::CapturePreFilteredTexture(CommandContext* context)
	{
		PIXScopedEvent(context->List.Get(), PIX_COLOR(0, 255, 0), "EnvSystem: Capture Prefiltered Texture");

		if (m_irradianceCalculated)
		{
			return;
		}

		context->TransitionResource(m_cubemap, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		context->TransitionResource(m_prefilteredMap, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_prefilteredMaterial->Set(context);

		for (std::size_t mip = 0; mip < k_munMipLevels; ++mip)
		{
			RoughnessParams rcb{ static_cast<float>(mip) / static_cast<float>(k_munMipLevels - 1) };

			context->SetDynamicCBV(1, sizeof(rcb), &rcb);

			LONG mipWidth = 128 * std::pow(0.5f, mip);
			LONG mipHeight = 128 * std::pow(0.5f, mip);

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

			D3D12_VIEWPORT viewport{ 0, 0, mipWidth, mipHeight };
			CD3DX12_RECT rect{ 0, 0, mipWidth, mipHeight };

			context->SetViewport(Viewport{ viewport, rect });

			CameraParams cameraParams;
			cameraParams.ProjMatrix = projMatrix;

			for (int i = 0; i < viewMatrices.size(); ++i)
			{
				context->List->OMSetRenderTargets(1, &m_prefilteredRTVs[i*k_munMipLevels+mip], FALSE, nullptr);

				cameraParams.ViewMatrix = viewMatrices[i];
				context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);

				m_cubeMesh->Draw(context);
			}
		}

		context->TransitionResource(m_prefilteredMap, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
	}

	void ecs::EnvironmentSystem::ConvoluteBRDF(CommandContext* context)
	{
		if (m_irradianceCalculated)
		{
			return;
		}

		PIXScopedEvent(context->List.Get(), PIX_COLOR(0, 255, 0), "EnvSystem: Convolute BRDF");

		auto* render = alexis::Render::GetInstance();
		auto* rtManager = render->GetRTManager();
		auto* rt = rtManager->GetRenderTarget(L"ConvolutedBRDF");

		context->TransitionResource(rt->GetTexture(RenderTarget::Slot0), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

		m_convoluteBRDFMaterial->Set(context);

		context->SetRenderTarget(*rt);
		context->SetViewport(rt->GetViewport());

		m_fsQuad->Draw(context);

		context->TransitionResource(rt->GetTexture(RenderTarget::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
	}

	void ecs::EnvironmentSystem::RenderSkybox(CommandContext* context)
	{
		PIXScopedEvent(context->List.Get(), PIX_COLOR(0, 0, 255), "EnvironmentSystem: Skybox");

		auto& ecsWorld = Core::Get().GetECSWorld();
		auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
		auto activeCamera = cameraSystem->GetActiveCamera();

		auto* render = alexis::Render::GetInstance();
		auto* rtManager = render->GetRTManager();

		auto* rt = rtManager->GetRenderTarget(L"HDR");
		auto* gb = rtManager->GetRenderTarget(L"GB");

		context->TransitionResource(rt->GetTexture(RenderTarget::Slot0), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
		context->TransitionResource(gb->GetTexture(RenderTarget::DepthStencil), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_READ);

		m_skyboxMaterial->Set(context);

		context->SetRenderTarget(*rt, *gb);
		context->SetViewport(rt->GetViewport());

		CameraParams cameraParams;
		cameraParams.ViewMatrix = cameraSystem->GetViewMatrix(activeCamera);
		cameraParams.ProjMatrix = cameraSystem->GetProjMatrix(activeCamera);
		context->SetDynamicCBV(0, sizeof(cameraParams), &cameraParams);

		m_cubeMesh->Draw(context);
	}

}
