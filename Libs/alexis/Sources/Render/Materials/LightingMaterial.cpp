#include <Precompiled.h>

#include "LightingMaterial.h"

#include <Render/Render.h>

// for VertexDef
#include <Render/Mesh.h>
#include <Render/CommandContext.h>

namespace alexis
{
	enum LightingParameters
	{
		CameraParams,
		DirectionalLights,
		ShadowMapParams,
		GBuffer, //Texture2D 3 textures starting from BaseColor : register( t0 );
		ShadowMap,
		NumLightingParameters
	};

	LightingMaterial::LightingMaterial()
	{
		ComPtr<ID3DBlob> vertexShaderBlob;
		ComPtr<ID3DBlob> pixelShaderBlob;
#if defined(_DEBUG)
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Lighting_vs_d.cso", &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Lighting_ps_d.cso", &pixelShaderBlob));
#else 
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Lighting_vs.cso", &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Lighting_ps.cso", &pixelShaderBlob));
#endif

		// Allow input layout and deny unnecessary access to certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
		CD3DX12_DESCRIPTOR_RANGE1 shadowMapDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);

		CD3DX12_ROOT_PARAMETER1 rootParameters[LightingParameters::NumLightingParameters];
		rootParameters[LightingParameters::CameraParams].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[LightingParameters::DirectionalLights].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[LightingParameters::ShadowMapParams].InitAsConstantBufferView(2, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[LightingParameters::GBuffer].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[LightingParameters::ShadowMap].InitAsDescriptorTable(1, &shadowMapDescriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC pointSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT);
		CD3DX12_STATIC_SAMPLER_DESC shadowSampler(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER);

		CD3DX12_STATIC_SAMPLER_DESC samplers[2] = { pointSampler, shadowSampler };

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(LightingParameters::NumLightingParameters, rootParameters, _countof(samplers), samplers, rootSignatureFlags);

		m_rootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

		CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

		// TODO create generic PSO class
		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vs;
			CD3DX12_PIPELINE_STATE_STREAM_PS ps;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
		} pipelineStateStream;

		auto render = Render::GetInstance();
		auto rtManager = render->GetRTManager();

		pipelineStateStream.rootSignature = m_rootSignature.GetRootSignature().Get();
		pipelineStateStream.inputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.rtvFormats = rtManager->GetRTFormats(L"HDR");
		pipelineStateStream.Rasterizer = rasterizerDesc;

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
		{
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		ThrowIfFailed(render->GetDevice()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));
	}

	void LightingMaterial::SetupToRender(CommandContext* commandContext)
	{
		// TODO: get RTs on init once instead getting every frame

		auto render = Render::GetInstance();
		auto rtManager = render->GetRTManager();
		auto gbuffer = rtManager->GetRenderTarget(L"GB");
		auto shadowMapRT = rtManager->GetRenderTarget(L"Shadow Map");
		auto hdr = rtManager->GetRenderTarget(L"HDR");

		auto& depth = gbuffer->GetTexture(RenderTarget::DepthStencil);
		auto& shadowMap = shadowMapRT->GetTexture(RenderTarget::DepthStencil);

		commandContext->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandContext->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot1), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandContext->TransitionResource(gbuffer->GetTexture(RenderTarget::Slot::Slot2), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandContext->TransitionResource(depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandContext->TransitionResource(shadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandContext->SetRenderTarget(*hdr);
		commandContext->SetViewport(hdr->GetViewport());
		//commandContext->List->RSSetScissorRects(1, &m_scissorRect);

		commandContext->SetPipelineState(m_pso.Get());
		commandContext->SetRootSignature(m_rootSignature);

		//commandContext->SetSRV(LightingParameters::GBuffer, 0, gbuffer->GetTexture(RenderTarget::Slot::Slot0));
		//commandContext->SetSRV(LightingParameters::GBuffer, 1, gbuffer->GetTexture(RenderTarget::Slot::Slot1));
		//commandContext->SetSRV(LightingParameters::GBuffer, 2, gbuffer->GetTexture(RenderTarget::Slot::Slot2));
		//commandContext->SetSRV(LightingParameters::GBuffer, 3, depth);
		
		//commandContext->SetSRV(LightingParameters::ShadowMap, 0, shadowMap);
	}

}

