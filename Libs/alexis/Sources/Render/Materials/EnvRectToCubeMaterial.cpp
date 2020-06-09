#include <Precompiled.h>

#include "EnvRectToCubeMaterial.h"

#include <Render/Render.h>

// for VertexDef
#include <Render/Mesh.h>
#include <Render/CommandContext.h>

namespace alexis
{
	enum EnvParams
	{
		CamCB,
		EnvMap,
		NumParams
	};

	EnvRectToCubeMaterial::EnvRectToCubeMaterial()
	{
		// TODO: move shader loading to resource manager

#if defined(_DEBUG)
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/EnvRectToCube_vs_d.cso", &m_vertexShader));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/EnvRectToCube_ps_d.cso", &m_pixelShader));
#else
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/EnvRectToCube_vs.cso", &m_vertexShader));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/EnvRectToCube_ps.cso", &m_pixelShader));
#endif

		// Allow input layout and deny unnecessary access to certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[EnvParams::NumParams];
		rootParameters[EnvParams::CamCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[EnvParams::EnvMap].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC pointSampler(0, D3D12_FILTER_MIN_MAG_MIP_POINT);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(EnvParams::NumParams, rootParameters, 1, &pointSampler, rootSignatureFlags);

		m_rootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

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

		CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

		auto* rtManager = Render::GetInstance()->GetRTManager();

		pipelineStateStream.rootSignature = m_rootSignature.GetRootSignature().Get();
		pipelineStateStream.inputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(m_vertexShader.Get());
		pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(m_pixelShader.Get());
		pipelineStateStream.rtvFormats = rtManager->GetRTFormats(L"HDR");
		pipelineStateStream.Rasterizer = rasterizerDesc;

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
		{
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		auto* render = Render::GetInstance();
		ThrowIfFailed(render->GetDevice()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));
	}

	void EnvRectToCubeMaterial::SetupToRender(CommandContext* context)
	{
		//auto* rtManager = Render::GetInstance()->GetRTManager();
		//auto* hdrRt = rtManager->GetRenderTarget(L"HDR");

		//context->SetRenderTarget(*hdrRt);
		//context->SetViewport(hdrRt->GetViewport());

		context->SetPipelineState(m_pso.Get());
		context->SetRootSignature(m_rootSignature);


		//pbsCommandContext->SetSRV(PBSObjectParameters::Textures, 0, *m_baseColor);
		//pbsCommandContext->SetSRV(PBSObjectParameters::Textures, 1, *m_normalMap);
		//pbsCommandContext->SetSRV(PBSObjectParameters::Textures, 2, *m_metalRoughness);
	}

}