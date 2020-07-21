#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <optional>

#include <Render/RootSignature.h>

namespace alexis
{
	class CommandContext;
	using Microsoft::WRL::ComPtr;

	struct MaterialLoadParams
	{
		std::wstring VSPath;
		std::wstring PSPath;

		std::vector<std::wstring> Textures;
		std::wstring RTV;
		bool DepthEnable;
		D3D12_CULL_MODE CullMode{ D3D12_CULL_MODE_BACK };
		D3D12_COMPARISON_FUNC DepthFunc{ D3D12_COMPARISON_FUNC_LESS };
		D3D12_DEPTH_WRITE_MASK DepthWriteMask{ D3D12_DEPTH_WRITE_MASK_ALL };
		bool CustomDS = false;
		CD3DX12_DEPTH_STENCIL_DESC DepthStencil{ D3D12_DEFAULT };
		CD3DX12_BLEND_DESC BlendDesc{ D3D12_DEFAULT };
	};

	class Material
	{
	public:
		Material(const MaterialLoadParams& params);

		const ID3D12RootSignature* GetRootSignature()const
		{
			return m_rootSignature.Get();
		}

		const ID3D12PipelineState* GetPipelineState() const
		{
			return m_pso.Get();
		}

		void Set(CommandContext* context);

	private:
		ComPtr<ID3D12RootSignature> m_rootSignature;
		ComPtr<ID3D12PipelineState> m_pso;

		int m_srvStartIndex{ 0 };
		std::optional<std::size_t> m_srvOffset;

		ComPtr<ID3DBlob> m_vertexShader;
		ComPtr<ID3DBlob> m_pixelShader;
	};
}