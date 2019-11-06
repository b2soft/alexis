#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <Render/RootSignature.h>
#include <Render/Buffers/GpuBuffer.h>

namespace alexis
{
	class IMaterial
	{
	public:

	};

	using Microsoft::WRL::ComPtr;
	
	struct PBRMaterialParams
	{
		std::wstring VSPath;
		std::wstring PSPath;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;


		// Temporary buffers
		TextureBuffer& t0;
		TextureBuffer& t1;
		TextureBuffer& t2;
	};

	class CommandContext;

	class PBRMaterial
	{
	public:
		PBRMaterial(const PBRMaterialParams& params);

		void SetupToRender(CommandContext* commandContext);

		const RootSignature& GetRootSignature()const
		{
			return m_rootSignature;
		}

		const ID3D12PipelineState* GetPipelineState() const
		{
			return m_pso.Get();
		}

	private:
		RootSignature m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pso;
		ComPtr<ID3DBlob> m_vertexShader;
		ComPtr<ID3DBlob> m_pixelShader;

		TextureBuffer& m_baseColor;
		TextureBuffer& m_normalMap;
		TextureBuffer& m_metalRoughness;
	};

}
