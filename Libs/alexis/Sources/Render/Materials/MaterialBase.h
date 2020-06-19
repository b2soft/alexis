#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <optional>

#include <Render/RootSignature.h>

namespace alexis
{
	class CommandContext;
	using Microsoft::WRL::ComPtr;

	class MaterialBase
	{
	public:
		virtual void SetupToRender(CommandContext* commandContext) = 0;
		virtual ~MaterialBase();

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

	struct MaterialLoadParams
	{
		std::wstring VSPath;
		std::wstring PSPath;

		std::vector<std::wstring> Textures;
		std::wstring RTV;
		bool DepthEnable;
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