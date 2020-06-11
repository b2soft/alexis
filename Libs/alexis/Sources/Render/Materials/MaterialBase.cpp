#include "Precompiled.h"
#include "MaterialBase.h"

#include <Render/Mesh.h>
#include <Render/Render.h>
#include <Render/CommandContext.h>
#include <Core/Core.h>
#include <Core/ResourceManager.h>

namespace alexis
{
	MaterialBase::~MaterialBase() = default;

	Material::Material(const MaterialLoadParams& params)
	{
		ComPtr<ID3DBlob> vertexShaderBlob;
		ComPtr<ID3DBlob> pixelShaderBlob;

#if defined(_DEBUG)
		std::wstring absVSPath = L"Resources/Shaders/" + params.VSPath + L"_vs_d.cso";
		std::wstring absPSPath = L"Resources/Shaders/" + params.PSPath + L"_ps_d.cso";
#else
		std::wstring absVSPath = L"Resources/Shaders/" + params.VSPath + L".cso";
		std::wstring absPSPath = L"Resources/Shaders/" + params.PSPath + L".cso";
#endif
		ThrowIfFailed(D3DReadFileToBlob(absVSPath.c_str(), &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(absPSPath.c_str(), &pixelShaderBlob));

		auto* render = Render::GetInstance();
		auto* device = render->GetDevice();
		auto result = device->CreateRootSignature(0, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

		auto* resMgr = Core::Get().GetResourceManager();
		for (auto& texturePath : params.Textures)
		{
			if (!texturePath.empty())
			{
				auto* texture = resMgr->GetTexture(texturePath);
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.Format = texture->GetResourceDesc().Format;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;

				auto srvAlloc = render->AllocateSRV(texture->GetResource(), srvDesc);

				if (!m_srvOffset.has_value())
				{
					m_srvOffset = srvAlloc.OffsetInHeap;
				}
			}
		}

		auto rtManager = render->GetRTManager();

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 3;
		psoDesc.RTVFormats[0] = rtManager->GetRenderTarget(L"GB")->GetFormat().RTFormats[0];
		psoDesc.RTVFormats[1] = rtManager->GetRenderTarget(L"GB")->GetFormat().RTFormats[1];
		psoDesc.RTVFormats[2] = rtManager->GetRenderTarget(L"GB")->GetFormat().RTFormats[2];
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(render->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pso)));


		//CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
		//rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

		//CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
		//

		//struct PipelineStateStream
		//{
		//	CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
		//	CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
		//	CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
		//	CD3DX12_PIPELINE_STATE_STREAM_VS vs;
		//	CD3DX12_PIPELINE_STATE_STREAM_PS ps;
		//	CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
		//	//CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;
		//	CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
		//	//CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormats;
		//} pipelineStateStream;

		//
		//

		//pipelineStateStream.rootSignature = m_rootSignature.Get();
		//pipelineStateStream.inputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		//pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		//pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		//pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		//pipelineStateStream.Rasterizer = rasterizerDesc;
		////pipelineStateStream.DepthStencil = depthStencilDesc;
		//pipelineStateStream.rtvFormats = rtManager->GetRenderTarget(L"GB")->GetFormat();

		//D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc{sizeof(PipelineStateStream), &pipelineStateStream};

		//ThrowIfFailed(render->GetDevice()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));




		//D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		//desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		//desc.NumDescriptors = 1;
		//desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		//ThrowIfFailed(Render::GetInstance()->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_imguiSrvHeap)));	
	}

	void Material::Set(CommandContext* context)
	{
		auto* render = Render::GetInstance();
		auto* device = render->GetDevice();

		context->List->SetPipelineState(m_pso.Get());
		context->List->SetGraphicsRootSignature(m_rootSignature.Get());

		if (m_srvOffset.has_value())
		{
			auto* srvHeap = render->GetSrvUavHeap();
			ID3D12DescriptorHeap* ppHeaps[] = { srvHeap };
			context->List->SetDescriptorHeaps(1, ppHeaps);

			CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle{ srvHeap->GetGPUDescriptorHandleForHeapStart() };
			gpuHandle.Offset(m_srvOffset.value(), device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

			context->List->SetGraphicsRootDescriptorTable(1, gpuHandle);
		}
	}

}
