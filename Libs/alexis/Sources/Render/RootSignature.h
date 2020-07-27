#pragma once

#include <d3dx12.h>
#include <wrl.h>
#include <vector>

namespace alexis
{

	class RootSignature
	{
	public:
		RootSignature();
		RootSignature(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion);

		virtual ~RootSignature();

		void Destroy();

		Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() const
		{
			return m_rootSignature;
		}

		void SetRootSignatureDesc(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION rootSignatureVersion);

		const D3D12_ROOT_SIGNATURE_DESC1& GetRootSignatureDesc() const
		{
			return m_rootSignatureDesc;
		}

		uint32_t GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType) const;
		uint32_t GetNumDescriptors(uint32_t rootIndex) const;

	private:
		D3D12_ROOT_SIGNATURE_DESC1 m_rootSignatureDesc;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

		// Need to know the number of descriptors per descriptor table
		// A maximum of 32 descriptor tables are supported (since a 32-bit mask is used to represent the descriptor tables in the root signature
		uint32_t m_numDescriptorsPerTable[32];

		// A bit mask that represents the root parameter indices that are 
		// CBV, UAV, and SRV descriptor tables.
		uint32_t m_descriptorTableBitMask;

		// A bit mask that represents the root parameter indices that are 
		// descriptor tables for Samplers.
		uint32_t m_samplerTableBitMask;
	};

}