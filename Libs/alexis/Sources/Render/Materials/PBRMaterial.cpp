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
		MatricesCB, //ConstantBuffer<Mat> MatCB: register(b0);
		Textures, //Texture2D 3 textures starting from BaseColor : register( t0 );
		NumPBSObjectParameters
	};

	PBRMaterial::PBRMaterial(const PBRMaterialParams& params) :
		m_baseColor(params.t0),
		m_normalMap(params.t1),
		m_metalRoughness(params.t2)
	{
		// TODO: move shader loading to resource manager

#if defined(_DEBUG)
		// debug only
		std::wstring debugSuffix = L"_d";

		std::wstring vsPathDebug = params.VSPath;
		vsPathDebug.insert(params.VSPath.find_last_of('.'), debugSuffix);

		std::wstring psPathDebug = params.PSPath;
		psPathDebug.insert(params.PSPath.find_last_of('.'), debugSuffix);

		ThrowIfFailed(D3DReadFileToBlob(vsPathDebug.c_str(), &m_vertexShader));
		ThrowIfFailed(D3DReadFileToBlob(psPathDebug.c_str(), &m_pixelShader));
#else
		ThrowIfFailed(D3DReadFileToBlob(params.VSPath.c_str(), &m_vertexShader));
		ThrowIfFailed(D3DReadFileToBlob(params.PSPath.c_str(), &m_pixelShader));
#endif

		// Allow input layout and deny unnecessary access to certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[PBSObjectParameters::NumPBSObjectParameters];
		rootParameters[PBSObjectParameters::MatricesCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[PBSObjectParameters::Textures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(PBSObjectParameters::NumPBSObjectParameters, rootParameters, 1, &anisotropicSampler, rootSignatureFlags);

		auto render = Render::GetInstance();

		// TODO: consider using constant highest signature version since only desc 1.1 is created
		m_rootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, render->GetHightestSignatureVersion());

		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vs;
			CD3DX12_PIPELINE_STATE_STREAM_PS ps;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
		} pipelineStateStream;

		pipelineStateStream.rootSignature = m_rootSignature.GetRootSignature().Get();
		pipelineStateStream.inputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(m_vertexShader.Get());
		pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(m_pixelShader.Get());
		pipelineStateStream.rtvFormats = params.RTVFormats;
		pipelineStateStream.dsvFormat = params.DSVFormat;

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
		{
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		ThrowIfFailed(render->GetDevice()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));
	}

	void PBRMaterial::SetupToRender(CommandContext* pbsCommandContext)
	{
		pbsCommandContext->SetPipelineState(m_pso.Get());
		pbsCommandContext->SetRootSignature(m_rootSignature);

		pbsCommandContext->SetSRV(PBSObjectParameters::Textures, 0, m_baseColor);
		pbsCommandContext->SetSRV(PBSObjectParameters::Textures, 1, m_normalMap);
		pbsCommandContext->SetSRV(PBSObjectParameters::Textures, 2, m_metalRoughness);
	}

}