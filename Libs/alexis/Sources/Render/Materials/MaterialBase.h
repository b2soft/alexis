#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <Render/RootSignature.h>

namespace alexis
{
	class CommandContext;
	using Microsoft::WRL::ComPtr;

	class MaterialBase
	{
	public:
		virtual void SetupToRender(CommandContext* commandContext) = 0;
		virtual ~MaterialBase() = default;

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
}