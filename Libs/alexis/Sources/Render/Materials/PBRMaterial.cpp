#include <Precompiled.h>

#include "PBRMaterial.h"

#include <Core/Render.h>

// for VertexDef
#include <Render/Mesh.h>
#include <Render/CommandContext.h>

namespace alexis
{
	enum PBSObjectParameters
	{
		CameraCB,
		ModelCB,
		Textures, //Texture2D 3 textures starting from BaseColor : register( t0 );
		NumPBSObjectParameters
	};

	PBRMaterial::PBRMaterial(const PBRMaterialParams& params) :
		m_baseColor(params.BaseColor),
		m_normalMap(params.NormalMap),
		m_metalRoughness(params.MetalRoughness)
	{
		// TODO: move shader loading to resource manager

#if defined(_DEBUG)
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PBS_object_vs_d.cso", &m_vertexShader));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PBS_object_ps_d.cso", &m_pixelShader));
#else
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PBS_object_vs.cso", &m_vertexShader));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PBS_object_ps.cso", &m_pixelShader));
#endif

		// Allow input layout and deny unnecessary access to certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[PBSObjectParameters::NumPBSObjectParameters];
		rootParameters[PBSObjectParameters::CameraCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[PBSObjectParameters::ModelCB].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[PBSObjectParameters::Textures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(PBSObjectParameters::NumPBSObjectParameters, rootParameters, 1, &anisotropicSampler, rootSignatureFlags);

		m_rootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, D3D_ROOT_SIGNATURE_VERSION_1_1);

		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vs;
			CD3DX12_PIPELINE_STATE_STREAM_PS ps;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL depthStencil;
		} pipelineStateStream;

		auto rtManager = Render::GetInstance()->GetRTManager();

		CD3DX12_DEPTH_STENCIL_DESC	dsDesc{ CD3DX12_DEFAULT() };
		dsDesc.StencilEnable = TRUE;
		dsDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR_SAT;

		pipelineStateStream.rootSignature = m_rootSignature.GetRootSignature().Get();
		pipelineStateStream.inputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(m_vertexShader.Get());
		pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(m_pixelShader.Get());
		pipelineStateStream.rtvFormats = rtManager->GetRTFormats(L"GB");
		pipelineStateStream.dsvFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;// rtManager->GetDSFormat(L"GB"); // 
		pipelineStateStream.depthStencil = dsDesc;

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
		{
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		auto render = Render::GetInstance();
		ThrowIfFailed(render->GetDevice()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));
	}

	void PBRMaterial::SetupToRender(CommandContext* pbsCommandContext)
	{
		pbsCommandContext->SetPipelineState(m_pso.Get());
		pbsCommandContext->SetRootSignature(m_rootSignature);

		pbsCommandContext->SetSRV(PBSObjectParameters::Textures, 0, *m_baseColor);
		pbsCommandContext->SetSRV(PBSObjectParameters::Textures, 1, *m_normalMap);
		pbsCommandContext->SetSRV(PBSObjectParameters::Textures, 2, *m_metalRoughness);
	}

}