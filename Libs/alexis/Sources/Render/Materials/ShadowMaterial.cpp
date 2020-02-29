#include <Precompiled.h>

#include "ShadowMaterial.h"

#include <Render/Render.h>

// for VertexDef
#include <Render/Mesh.h>
#include <Render/CommandContext.h>

namespace alexis
{
	enum ShadowParameters
	{
		ShadowParams,
		NumShadowParameters
	};

	ShadowMaterial::ShadowMaterial()
	{
		ComPtr<ID3DBlob> vertexShaderBlob;
#if defined(_DEBUG)
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Shadows_vs_d.cso", &vertexShaderBlob));
#else 
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Shadows_vs.cso", &vertexShaderBlob));
#endif

		// Allow input layout and deny unnecessary access to certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_ROOT_PARAMETER1 rootParameters[ShadowParameters::NumShadowParameters];
		rootParameters[ShadowParameters::ShadowParams].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

		CD3DX12_STATIC_SAMPLER_DESC pointSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(ShadowParameters::NumShadowParameters, rootParameters, 1, &pointSampler, rootSignatureFlags);

		m_rootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

		// TODO create generic PSO class
		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vs;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormat;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL depthStencil;
		} pipelineStateStream;

		auto render = Render::GetInstance();
		auto rtManager = render->GetRTManager();
		auto dsFormat = rtManager->GetDSFormat(L"Shadow Map");

		CD3DX12_DEPTH_STENCIL_DESC	dsDesc{ CD3DX12_DEFAULT() };

		pipelineStateStream.rootSignature = m_rootSignature.GetRootSignature().Get();
		pipelineStateStream.inputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.dsvFormat = pipelineStateStream.dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		pipelineStateStream.depthStencil = dsDesc;

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
		{
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		ThrowIfFailed(render->GetDevice()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));
	}

	void ShadowMaterial::SetupToRender(CommandContext* commandContext)
	{
		// TODO: get RTs on init once instead getting every frame

		auto render = Render::GetInstance();
		auto rtManager = render->GetRTManager();
		auto shadowRT = rtManager->GetRenderTarget(L"Shadow Map");

		//auto& depth = shadowRT->GetTexture(RenderTarget::DepthStencil);
		//
		//commandContext->TransitionResource(depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandContext->SetRenderTarget(*shadowRT);
		commandContext->SetViewport(shadowRT->GetViewport());

		commandContext->SetPipelineState(m_pso.Get());
		commandContext->SetRootSignature(m_rootSignature);
	}

}

