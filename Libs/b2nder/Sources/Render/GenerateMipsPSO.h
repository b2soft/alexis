#pragma once

#include "RootSignature.h"
#include "DescriptorAllocation.h"

#include <d3d12.h>
#include <DirectXMath.h>
#include <wrl.h>

struct alignas(16) GenerateMipsCB
{
	uint32_t SrcMipLevel; // Texture level of source mip
	uint32_t NumMipLevels; // Number of OutMips to write [1-4]
	uint32_t SrcDimension; // Width and height of the src texture
	uint32_t IsSRGB; //Must apply gamma correction to sRGB textures
	DirectX::XMFLOAT2 TexelSize; //1.0 / OutMip1.Dimensions
};

namespace GenerateMips
{
	enum
	{
		GenerateMipsCB,
		SrcMip,
		OutMip,
		NumRootParameters
	};
}

class GenerateMipsPSO
{
public:
	GenerateMipsPSO();

	const RootSignature& GetRootSignature() const
	{
		return m_rootSignature;
	}

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState() const
	{
		return m_pipelineState;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetDefaultUAV() const
	{
		return m_defaultUAV.GetDescriptorHandle();
	}

private:
	RootSignature m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	// Default (no resource) UAV's to pad the unused UAV descriptors
	// If generating less than 4 mip map levels, the unused mip maps
	// need to be padded with default UAVs (to keep the DX12 runtime happy)
	DescriptorAllocation m_defaultUAV;
};