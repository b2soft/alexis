#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <Render/RootSignature.h>
#include <Render/Buffers/GpuBuffer.h>

namespace alexis
{
	class CommandContext;
	using Microsoft::WRL::ComPtr;

	class IMaterial
	{
	public:
		virtual void SetupToRender(CommandContext* commandContext) = 0;

		const RootSignature& GetRootSignature()const
		{
			return m_rootSignature;
		}

		const ID3D12PipelineState* GetPipelineState() const
		{
			return m_pso.Get();
		}

	protected:
		RootSignature m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pso;

		ComPtr<ID3DBlob> m_vertexShader;
		ComPtr<ID3DBlob> m_pixelShader;
	};


	struct PBRMaterialParams
	{
		TextureBuffer* BaseColor;
		TextureBuffer* NormalMap;
		TextureBuffer* MetalRoughness;
	};

	class PBRMaterial : public IMaterial
	{
	public:
		PBRMaterial(const PBRMaterialParams& params);

		virtual void SetupToRender(CommandContext* commandContext) override;

	private:
		TextureBuffer* m_baseColor{ nullptr };
		TextureBuffer* m_normalMap{ nullptr };
		TextureBuffer* m_metalRoughness{ nullptr };
	};

}
