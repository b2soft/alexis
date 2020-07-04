#include <Precompiled.h>

#include "LightingSystem.h"

#include <ECS/ECS.h>
#include <ECS/Systems/CameraSystem.h>
#include <ECS/Systems/ShadowSystem.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/LightComponent.h>

#include <Core/Core.h>
#include <Core/ResourceManager.h>
#include <Render/Render.h>
#include <Render/CommandContext.h>

#include <sstream>

namespace alexis
{
	namespace ecs
	{
		// Todo: generalize common case
		__declspec(align(16)) struct CameraParams
		{
			XMVECTOR CameraPos;
			XMMATRIX InvViewMatrix;
			XMMATRIX InvProjMatrix;
		};

		__declspec(align(16)) struct DirectionalLight
		{
			XMVECTOR Direction;
			XMVECTOR Color;
		};

		__declspec(align(16)) struct ShadowMapParams
		{
			XMMATRIX LightSpaceMatrix;
		};

		void LightingSystem::Init()
		{
			auto* resMgr = Core::Get().GetResourceManager();
			m_lightingMaterial = resMgr->GetMaterial(L"Resources/Materials/system/Lighting.material");
			m_buildClustersMaterial = resMgr->GetMaterial(L"Resources/Materials/system/clustered_forward/build_clusters.material");

			m_fsQuad = Core::Get().GetResourceManager()->GetMesh(L"$FS_QUAD");
		}

		void LightingSystem::Render(CommandContext* context)
		{
			auto* render = Render::GetInstance();
			auto* rtManager = render->GetRTManager();
			auto* gbuffer = rtManager->GetRenderTarget(L"GB");
			auto* shadowMapRT = rtManager->GetRenderTarget(L"Shadow Map");
			auto* hdr = rtManager->GetRenderTarget(L"HDR");
			auto* irradiance = rtManager->GetRenderTarget(L"CUBEMAP_Irradiance");
			auto* prefiltered = rtManager->GetRenderTarget(L"CUBEMAP_Prefiltered");
			auto* convBRDF = rtManager->GetRenderTarget(L"ConvolutedBRDF");

			auto& depth = gbuffer->GetTexture(RenderTarget::DepthStencil);
			auto& shadowMap = shadowMapRT->GetTexture(RenderTarget::DepthStencil);

			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot1), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot2), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(shadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(irradiance->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(prefiltered->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			context->TransitionResource(convBRDF->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			context->SetRenderTarget(*hdr, *gbuffer);
			context->SetViewport(hdr->GetViewport());

			m_lightingMaterial->Set(context);

			auto& ecsWorld = Core::Get().GetECSWorld();
			auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
			auto activeCamera = cameraSystem->GetActiveCamera();

			auto& transformComponent = ecsWorld.GetComponent<TransformComponent>(activeCamera);

			CameraParams cameraParams;
			cameraParams.CameraPos = transformComponent.Position;
			cameraParams.InvViewMatrix = cameraSystem->GetInvViewMatrix(activeCamera);
			cameraParams.InvProjMatrix = cameraSystem->GetInvProjMatrix(activeCamera);
			context->SetCBV(0, sizeof(cameraParams), &cameraParams);

			DirectionalLight directionalLights[1];

			for (const auto& entity : Entities)
			{
				auto& lightComponent = ecsWorld.GetComponent<LightComponent>(entity);
				if (lightComponent.Type == ecs::LightComponent::LightType::Directional)
				{
					directionalLights[0].Color = lightComponent.Color;
					directionalLights[0].Direction = lightComponent.Direction;
				}
			}

			context->SetCBV(1, sizeof(directionalLights), &directionalLights);

			ShadowMapParams depthParams;

			auto shadowSystem = ecsWorld.GetSystem<ShadowSystem>();
			depthParams.LightSpaceMatrix = shadowSystem->GetShadowMatrix();

			context->SetCBV(2, sizeof(depthParams), &depthParams);

			m_fsQuad->Draw(context);
		}

		__declspec(align(16)) struct ClusterData
		{
			XMFLOAT4 MinPoint;
			XMFLOAT4 MaxPoint;
		};

		__declspec(align(16)) struct ScreenToViewParams
		{
			XMMATRIX InvProj;
			XMUINT4 ClusterSize;
			XMUINT2 ScreenSize;
		};

		__declspec(align(16))  struct ClusterCameraParams
		{
			float Near;
			float Far;
		};

		static inline UINT AlignForUavCounter(UINT bufferSize)
		{
			const UINT alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
			return (bufferSize + (alignment - 1)) & ~(alignment - 1);
		}

		void LightingSystem::BuildClusters(CommandContext* context)
		{
			auto* render = Render::GetInstance();
			auto* device = render->GetDevice();

			if (!m_uavOffset.has_value())
			{
				const UINT uavBufferSize = 16 * 9 * 24 * sizeof(ClusterData);
				const UINT counterOffset = AlignForUavCounter(uavBufferSize);

				D3D12_RESOURCE_DESC uavResDesc = CD3DX12_RESOURCE_DESC::Buffer(counterOffset + sizeof(UINT), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
				auto* properties = &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
				device->CreateCommittedResource(properties, D3D12_HEAP_FLAG_NONE, &uavResDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_uavResource));

				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
				uavDesc.Format = DXGI_FORMAT_UNKNOWN;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				uavDesc.Buffer.FirstElement = 0;
				uavDesc.Buffer.NumElements = 16 * 9 * 24;
				uavDesc.Buffer.StructureByteStride = sizeof(ClusterData);
				uavDesc.Buffer.CounterOffsetInBytes = counterOffset;
				uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

				auto alloc = render->AllocateUAV(m_uavResource.Get(), uavDesc);
				m_uavHandle = alloc.CpuPtr;
				m_uavOffset = alloc.OffsetInHeap;

				const UINT byteSize = counterOffset + sizeof(UINT);

				device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(byteSize), D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_uavReadback));
			}

			if (m_uavOffset.has_value())
			{
				PIXScopedEvent(context->List.Get(), PIX_COLOR(255, 0, 0), L"Build Clusters");
				m_buildClustersMaterial->Set(context);

				auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_uavResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
				context->List->ResourceBarrier(1, &barrier);

				auto* uavHeap = render->GetSrvUavHeap();
				ID3D12DescriptorHeap* ppHeaps[] = { uavHeap };
				context->List->SetDescriptorHeaps(1, ppHeaps);

				CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle{ uavHeap->GetGPUDescriptorHandleForHeapStart() };
				gpuHandle.Offset(m_uavOffset.value(), device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

				auto& ecsWorld = Core::Get().GetECSWorld();
				auto cameraSystem = ecsWorld.GetSystem<CameraSystem>();
				auto activeCamera = cameraSystem->GetActiveCamera();

				auto invProjMatrix = cameraSystem->GetInvProjMatrix(activeCamera);

				ScreenToViewParams stvParams;
				stvParams.InvProj = invProjMatrix;
				stvParams.ScreenSize = { 1280, 720 };
				UINT sizeX = (unsigned int)std::ceilf(1280 / (float)16);
				stvParams.ClusterSize = { 16, 9, 24, sizeX };

				context->SetComputeCBV(0, sizeof(stvParams), &stvParams);

				auto& cameraComponent = ecsWorld.GetComponent<CameraComponent>(activeCamera);

				ClusterCameraParams cameraParams;
				cameraParams.Near = cameraComponent.NearZ;
				cameraParams.Far = cameraComponent.FarZ;

				context->SetComputeCBV(1, sizeof(cameraParams), &cameraParams);

				//context->List->SetComputeRootUnorderedAccessView(2, m_uavResource->GetGPUVirtualAddress());
				context->List->SetComputeRootDescriptorTable(2, gpuHandle);

				context->List->Dispatch(16, 9, 24);

				context->List->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_uavResource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE));
				context->List->CopyResource(m_uavReadback.Get(), m_uavResource.Get());
				context->List->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_uavResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON));
			}
		}

		void LightingSystem::ReadClusters()
		{
			printf("TEST");

			auto* render = Render::GetInstance();
			auto* device = render->GetDevice();

			std::array<ClusterData, 16 * 9 * 24> data;
			m_uavReadback->Map(0, nullptr, reinterpret_cast<void**>(data.data()));
			for (auto& item : data)
			{
				std::wstringstream oss;

				oss << item.MinPoint.x << ", " << item.MinPoint.y << ", " << item.MinPoint.z << "; "
					<< item.MaxPoint.x << ", " << item.MaxPoint.y << ", " << item.MaxPoint.z << std::endl << std::endl;

				std::wstring ws = oss.str();
				OutputDebugString(ws.c_str());
			}
			m_uavReadback->Unmap(0, nullptr);
		}
	}
}
