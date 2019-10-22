#pragma once

#include <cstdint>
#include <mutex>
#include <memory>
#include <set>
#include <vector>

#include <d3dx12.h>

#include "DescriptorAllocation.h"

namespace alexis
{
	class DescriptorAllocatorPage;

	class DescriptorAllocator
	{
	public:
		DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256);
		virtual ~DescriptorAllocator();

		// Allocate a number of contiguous descriptors from a CPU visible descriptor heap
		DescriptorAllocation Allocate(uint32_t numDescriptors = 1);

		// Release stale descriptors when frame is completed
		void ReleaseStaleDescriptors(uint64_t frameNumber);

	private:
		using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

		// Create a new heap with a specific number of descriptors
		std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

		D3D12_DESCRIPTOR_HEAP_TYPE m_heapType;
		uint32_t m_numDescriptorsPerHeap;

		DescriptorHeapPool m_heapPool;
		// Indices of available heaps in the heap pool
		std::set<size_t> m_availableHeaps;

		std::mutex m_allocationMutex;
	};
}