#include <Precompiled.h>

#include "Hdr2SdrMaterial.h"

#include <Core/Render.h>

// for VertexDef
#include <Render/Mesh.h>
#include <Render/CommandContext.h>

namespace alexis
{
	enum Hdr2SdrParameters
	{
		HDR, //Texture2D 3 textures starting from BaseColor : register( t0 );
		NumHdr2SdrParameters
	};

	Hdr2SdrMaterial::Hdr2SdrMaterial()
	{
		// Load the vertex shader
		ComPtr<ID3DBlob> vertexShaderBlob;
		ComPtr<ID3DBlob> pixelShaderBlob;
#if defined(_DEBUG)
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Hdr2Sdr_vs_d.cso", &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Hdr2Sdr_ps_d.cso", &pixelShaderBlob));
#else
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Hdr2Sdr_vs.cso", &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Hdr2Sdr_ps.cso", &pixelShaderBlob));
#endif

		// Allow input layout and deny unnecessary access to certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[Hdr2SdrParameters::NumHdr2SdrParameters];
		rootParameters[Hdr2SdrParameters::HDR].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC linearSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(Hdr2SdrParameters::NumHdr2SdrParameters, rootParameters, 1, &linearSampler, rootSignatureFlags);

		m_rootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

		CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vs;
			CD3DX12_PIPELINE_STATE_STREAM_PS ps;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
		} pipelineStateStream;

		auto render = Render::GetInstance();
		auto rtManager = render->GetRTManager();

		pipelineStateStream.rootSignature = m_rootSignature.GetRootSignature().Get();
		pipelineStateStream.inputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.Rasterizer = rasterizerDesc;
		pipelineStateStream.rtvFormats = render->GetBackbufferRT().GetFormat();

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
		{
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		ThrowIfFailed(render->GetDevice()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));
	}

	void Hdr2SdrMaterial::SetupToRender(CommandContext* commandContext)
	{
		auto render = Render::GetInstance();
		auto rtManager = render->GetRTManager();
		auto hdrRT = rtManager->GetRenderTarget(L"HDR");

		commandContext->SetPipelineState(m_pso.Get());
		commandContext->SetRootSignature(m_rootSignature);

		commandContext->TransitionResource(hdrRT->GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandContext->SetSRV(Hdr2SdrParameters::HDR, 0, hdrRT->GetTexture(RenderTarget::Slot::Slot0));
	}

}
