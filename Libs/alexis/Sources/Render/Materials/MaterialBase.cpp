#include "Precompiled.h"
#include "MaterialBase.h"

#include <Render/Mesh.h>
#include <Render/Render.h>
#include <Render/CommandContext.h>
#include <Core/Core.h>
#include <Core/ResourceManager.h>

#include <Utils/RenderUtils.h>

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

		// Root Signature
		auto* render = Render::GetInstance();
		auto* device = render->GetDevice();
		auto result = device->CreateRootSignature(0, vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));

		Microsoft::WRL::ComPtr<ID3D12VersionedRootSignatureDeserializer> deserializer;
		HRESULT hs = D3D12CreateVersionedRootSignatureDeserializer(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), IID_PPV_ARGS(&deserializer));
		auto desc = deserializer->GetUnconvertedRootSignatureDesc()->Desc_1_1;

		for (auto i = 0; i < desc.NumParameters; ++i)
		{
			if (desc.pParameters[i].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			{
				m_srvStartIndex = i;
				break;
			}
		}


		auto* rtManager = render->GetRTManager();

		// Allocate SRVs in heap
		auto* resMgr = Core::Get().GetResourceManager();
		for (auto& texturePath : params.Textures)
		{
			if (!texturePath.empty())
			{
				//TODO: move it to loader?
				const alexis::TextureBuffer* texture = nullptr;
				// RenderTarget
				if (texturePath[0] == L'$')
				{
					auto indexPos = texturePath.find(L'#');
					auto rtName = texturePath.substr(1, indexPos - 1);

					int index = 0;

					if (indexPos != std::wstring::npos)
					{
						auto indexStr = texturePath.substr(indexPos + 1);
						if (indexStr == L"Depth")
						{
							index = RenderTarget::Slot::DepthStencil;
						}
						else
						{
							index = _wtoi(indexStr.c_str());
						}
					}

					texture = &rtManager->GetRenderTarget(rtName)->GetTexture(static_cast<RenderTarget::Slot>(index));
				}
				else
				{
					texture = resMgr->GetTexture(texturePath);
				}

				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srvDesc.Format = utils::GetFormatForSrv(texture->GetResourceDesc().Format);
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;

				auto srvAlloc = render->AllocateSRV(texture->GetResource(), srvDesc);

				if (!m_srvOffset.has_value())
				{
					m_srvOffset = srvAlloc.OffsetInHeap;
				}
			}
		}

		// PSO
		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS VS;
			CD3DX12_PIPELINE_STATE_STREAM_PS PS;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RtvFormats;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DsvFormats;
		} pipelineStateStream;

		CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
		CD3DX12_DEPTH_STENCIL_DESC depthStencilDesc(D3D12_DEFAULT);
		depthStencilDesc.DepthEnable = params.DepthEnable;

		pipelineStateStream.RootSignature = m_rootSignature.Get();
		pipelineStateStream.InputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		pipelineStateStream.PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.Rasterizer = rasterizerDesc;
		pipelineStateStream.DepthStencil = depthStencilDesc;

		//TODO: replace system key names
		if (params.RTV == L"$backbuffer")
		{
			pipelineStateStream.RtvFormats = render->GetBackbufferRT().GetFormat();
		}
		else
		{
			pipelineStateStream.RtvFormats = render->GetRTManager()->GetRenderTarget(params.RTV.substr(1))->GetFormat();
		}

		pipelineStateStream.DsvFormats = DXGI_FORMAT_D24_UNORM_S8_UINT;

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc{ sizeof(PipelineStateStream), &pipelineStateStream };

		ThrowIfFailed(render->GetDevice()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pso)));
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

			context->List->SetGraphicsRootDescriptorTable(m_srvStartIndex, gpuHandle);
		}
	}

}
