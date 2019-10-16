#include <Precompiled.h>

#include <cassert>
#include <stdexcept>

#include "DynamicDescriptorHeap.h"

#include <Core/Render.h>

namespace alexis
{

	DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint32_t numDescriptorsPerHeap /*= 1024*/) :
		m_descriptorHeapType(heapType),
		m_numDescriptorsPerHeap(numDescriptorsPerHeap),
		m_descriptorTableBitMask(0),
		m_staleDescriptorTableBitMask(0),
		m_currentCPUDescriptorHandle(D3D12_DEFAULT),
		m_currentGPUDescriptorHandle(D3D12_DEFAULT),
		m_numFreeHandles(0)
	{
		m_descriptorHandleIncrementSize = Render::GetInstance()->GetDevice()->GetDescriptorHandleIncrementSize(heapType);

		// Allocate space for staging CPU visible descriptors
		m_descriptorHandleCache = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(m_numDescriptorsPerHeap);
	}

	DynamicDescriptorHeap::~DynamicDescriptorHeap()
	{
	}

	void DynamicDescriptorHeap::StageDescriptors(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor)
	{
		// Cannot stage more than the maximum number of descriptors per heap
		// Cannot stage more than MaxDescriptorTables root parameters
		if (numDescriptors > m_numDescriptorsPerHeap || rootParameterIndex >= k_maxDescriptorTables)
		{
			throw std::bad_alloc();
		}

		DescriptorTableCache& descriptorTableCache = m_descriptorTableCache[rootParameterIndex];

		// Check that the number of descriptors to copy does not exceed the number
		// of descriptors expected in the descriptor table.
		if ((offset + numDescriptors) > descriptorTableCache.NumDescriptors)
		{
			throw std::length_error("Number of descriptors exceeds the number of descriptors in the descriptor table.");
		}

		D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = (descriptorTableCache.BaseDescriptor + offset);
		for (uint32_t i = 0; i < numDescriptors; ++i)
		{
			dstDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(srcDescriptor, i, m_descriptorHandleIncrementSize);
		}

		// Set the root parameter index bit to make sure the descriptor table at that index is bound to the command list
		m_staleDescriptorTableBitMask |= (1 << rootParameterIndex);
	}

	void DynamicDescriptorHeap::CommitStagedDescriptors(ID3D12GraphicsCommandList* commandList, std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc)
	{
		// Compute the number of descriptors that need to be copied
		uint32_t numDescriptorsToCommit = ComputeStaleDescriptorCount();

		if (numDescriptorsToCommit > 0)
		{
			auto device = Render::GetInstance()->GetDevice();
			assert(commandList != nullptr);

			if (!m_currentDescriptorHeap || m_numFreeHandles < numDescriptorsToCommit)
			{
				m_currentDescriptorHeap = RequestDescriptorHeap();
				m_currentCPUDescriptorHandle = m_currentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				m_currentGPUDescriptorHandle = m_currentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
				m_numFreeHandles = m_numDescriptorsPerHeap;

				// TODO
				//commandList->SetDescriptorHeap(m_descriptorHeapType, m_currentDescriptorHeap.Get());

				// When updating the descriptor heap on the command list, all descriptor
				// tables must be (re)recopied to the new descriptor heap (not just the stale descriptor tables)
				m_staleDescriptorTableBitMask = m_descriptorTableBitMask;
			}

			DWORD rootIndex;
			// Scan from LSB to MSB for a bit set in staleDescriptorsBitMask
			while (_BitScanForward(&rootIndex, m_staleDescriptorTableBitMask))
			{
				UINT numSrcDescriptors = m_descriptorTableCache[rootIndex].NumDescriptors;
				D3D12_CPU_DESCRIPTOR_HANDLE* srcDescriptorHandles = m_descriptorTableCache[rootIndex].BaseDescriptor;

				D3D12_CPU_DESCRIPTOR_HANDLE destDescriptorRangeStarts[] =
				{
					m_currentCPUDescriptorHandle
				};
				UINT destDescriptorRangeSizes[] =
				{
					numSrcDescriptors
				};

				// Copy the staged CPU visible descriptors to the GPU visible descriptor heap.
				device->CopyDescriptors(1, destDescriptorRangeStarts, destDescriptorRangeSizes,
					numSrcDescriptors, srcDescriptorHandles, nullptr, m_descriptorHeapType);

				// Set the descriptors on the command list using the passed-in setter function
				setFunc(commandList, rootIndex, m_currentGPUDescriptorHandle);

				// Offset current CPU and GPU descriptor handles.
				m_currentCPUDescriptorHandle.Offset(numSrcDescriptors, m_descriptorHandleIncrementSize);
				m_currentGPUDescriptorHandle.Offset(numSrcDescriptors, m_descriptorHandleIncrementSize);
				m_numFreeHandles -= numSrcDescriptors;

				// Flip the stale bit so the descriptor table is not recopied again unless it is updated with a new descriptor
				m_staleDescriptorTableBitMask ^= (1 << rootIndex);
			}
		}
	}

	void DynamicDescriptorHeap::CommitStagedDescriptorsForDraw(ID3D12GraphicsCommandList* commandList)
	{
		CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
	}

	void DynamicDescriptorHeap::CommitStagedDescriptorsForDispatch(ID3D12GraphicsCommandList* commandList)
	{
		CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::CopyDescriptor(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor)
	{
		if (!m_currentDescriptorHeap || m_numFreeHandles < 1)
		{
			m_currentDescriptorHeap = RequestDescriptorHeap();
			m_currentCPUDescriptorHandle = m_currentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			m_currentGPUDescriptorHandle = m_currentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			m_numFreeHandles = m_numDescriptorsPerHeap;

			// TODO
			//commandList.SetDescriptorHeap(m_descriptorHeapType, m_currentDescriptorHeap.Get());

			// When updating the descriptor heap on the command list, all descriptor
			// tables must be (re)recopied to the new descriptor heap (not just
			// the stale descriptor tables).
			m_staleDescriptorTableBitMask = m_descriptorTableBitMask;
		}

		auto device = Render::GetInstance()->GetDevice();

		D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = m_currentGPUDescriptorHandle;
		device->CopyDescriptorsSimple(1, m_currentCPUDescriptorHandle, cpuDescriptor, m_descriptorHeapType);

		m_currentCPUDescriptorHandle.Offset(1, m_descriptorHandleIncrementSize);
		m_currentGPUDescriptorHandle.Offset(1, m_descriptorHandleIncrementSize);
		m_numFreeHandles -= 1;

		return handleGPU;
	}

	//void DynamicDescriptorHeap::ParseRootSignature(const RootSignature& rootSignature)
	//{
	//	// If the root signature changes, all descriptors must be (re)bound to the command list
	//	m_staleDescriptorTableBitMask = 0;
	//
	//	const auto& rootSignatureDesc = rootSignature.GetRootSignatureDesc();
	//
	//	// Get a bit mask that represents the root parameter indices that match the descriptor heap type for this dynamic descriptor heap
	//	m_descriptorTableBitMask = rootSignature.GetDescriptorTableBitMask(m_descriptorHeapType);
	//	uint32_t descriptorTableBitMask = m_descriptorTableBitMask;
	//
	//	uint32_t currentOffset = 0;
	//	DWORD rootIndex;
	//	while (_BitScanForward(&rootIndex, descriptorTableBitMask) && rootIndex < rootSignatureDesc.NumParameters)
	//	{
	//		uint32_t numDescriptors = rootSignature.GetNumDescriptors(rootIndex);
	//
	//		DescriptorTableCache& descriptorTableCache = m_descriptorTableCache[rootIndex];
	//		descriptorTableCache.NumDescriptors = numDescriptors;
	//		descriptorTableCache.BaseDescriptor = m_descriptorHandleCache.get() + currentOffset;
	//
	//		currentOffset += numDescriptors;
	//
	//		// Flip the descriptor table bit so it's not scanned again for the current index
	//		descriptorTableBitMask ^= (1 << rootIndex);
	//	}
	//
	//	// Make sure the maximum number of descriptors per descriptor heap has not been exceeded
	//	assert(currentOffset <= m_numDescriptorsPerHeap && "The root signature requires more than the maximum number of descriptors per descriptor heap. Consider increasing the maximum number of descriptors per descriptor heap.");
	//}

	void DynamicDescriptorHeap::Reset()
	{
		m_availableDescriptorHeaps = m_descriptorHeapPool;
		m_currentDescriptorHeap.Reset();
		m_currentCPUDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
		m_currentGPUDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
		m_numFreeHandles = 0;
		m_descriptorTableBitMask = 0;
		m_staleDescriptorTableBitMask = 0;

		// Reset the table cache
		for (int i = 0; i < k_maxDescriptorTables; ++i)
		{
			m_descriptorTableCache[i].Reset();
		}
	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DynamicDescriptorHeap::RequestDescriptorHeap()
	{
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		if (!m_availableDescriptorHeaps.empty())
		{
			descriptorHeap = m_availableDescriptorHeaps.front();
			m_availableDescriptorHeaps.pop();
		}
		else
		{
			descriptorHeap = CreateDescriptorHeap();
			m_descriptorHeapPool.push(descriptorHeap);
		}

		return descriptorHeap;
	}

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DynamicDescriptorHeap::CreateDescriptorHeap()
	{
		auto device = Render::GetInstance()->GetDevice();

		D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
		descriptorHeapDesc.Type = m_descriptorHeapType;
		descriptorHeapDesc.NumDescriptors = m_numDescriptorsPerHeap;
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
		ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

		return descriptorHeap;
	}

	uint32_t DynamicDescriptorHeap::ComputeStaleDescriptorCount() const
	{
		uint32_t numStaleDescriptors = 0;
		DWORD i;
		DWORD staleDescriptorsBitMask = m_staleDescriptorTableBitMask;

		while (_BitScanForward(&i, staleDescriptorsBitMask))
		{
			numStaleDescriptors += m_descriptorTableCache[i].NumDescriptors;
			staleDescriptorsBitMask ^= (1 << i);
		}

		return numStaleDescriptors;
	}

}