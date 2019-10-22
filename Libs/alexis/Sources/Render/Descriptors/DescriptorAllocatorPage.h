#pragma once

#include <d3d12.h>

#include <wrl.h>

#include <map>
#include <memory>
#include <mutex>
#include <queue>

#include "DescriptorAllocation.h"

// Inspired by Variable Size Memory Allocations Manager
// http://diligentgraphics.com/diligent-engine/architecture/d3d12/variable-size-memory-allocations-manager/
// https://www.3dgep.com/learning-directx12-3/

namespace alexis
{

	class DescriptorAllocatorPage : public std::enable_shared_from_this<DescriptorAllocatorPage>
	{
	public:
		DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

		D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;

		// Check to see if this descriptor page has a contiguous block of descriptors large enough to satisfy the request
		bool HasSpace(uint32_t numDescriptors) const;

		// Get the number of available handles in the heap
		uint32_t NumFreeHandles() const;

		// Allocate a number of descriptors from this descriptor heap
		// If the allocation cannot be satisfied, then a NULL descriptor is returned
		DescriptorAllocation Allocate(uint32_t numDescriptors);

		// Return a descriptor back to the heap
		// Stale descriptors are not freed directly, but put on a stale allocations queue
		// Stale allocations are returned to the heap using the DescriptorAllocatorPage::ReleaseStaleAllocations method
		void Free(DescriptorAllocation&& descriptor, uint64_t frameNumber);

		// Returned the stale descriptors back to the descriptor heap
		void ReleaseStaleDescriptors(uint64_t frameNumber);

	protected:
		// Compute the offset of the descriptor handle from the heap start
		uint32_t ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);

		// Adds a new block to free list
		void AddNewBlock(uint32_t offset, uint32_t numDescriptors);

		// Free a block of descriptors
		// This will also merge free blocks in the free list to form larger blocks that can be reused
		void FreeBlock(uint32_t offset, uint32_t numDescriptors);

	private:
		// The offset (in descriptors) within the descriptor heap
		using OffsetType = uint32_t;

		// The number of descriptors that are available
		using SizeType = uint32_t;

		struct FreeBlockInfo;

		// A map that lists the free blocks by the offset within the descriptor heap
		using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;

		// A map that lists the free blocks by size
		// Needs to be a multimap since multiple blocks can have the same sizes
		using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

		struct FreeBlockInfo
		{
			FreeBlockInfo(SizeType size)
				: Size(size)
			{}

			SizeType Size;
			FreeListBySize::iterator FreeListBySizeIt;
		};

		struct StaleDescriptorInfo
		{
			StaleDescriptorInfo(OffsetType offset, SizeType size, uint64_t frameNumber)
				: Offset(offset)
				, Size(size)
				, FrameNumber(frameNumber)
			{}

			// Offset within the descriptor heap
			OffsetType Offset;

			// Number of descriptors
			SizeType Size;

			// Frame number that the descriptor was freed
			uint64_t FrameNumber;
		};

		// Stale descriptors are queued for release until the frame that they were freed has completed
		using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;

		FreeListByOffset m_freeListByOffset;
		FreeListBySize m_freeListBySize;
		StaleDescriptorQueue m_staleDescriptors;

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_d3d12DescriptorHeap;
		D3D12_DESCRIPTOR_HEAP_TYPE m_heapType;
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_baseDescriptor;
		uint32_t m_descriptorHandleIncrementSize;
		uint32_t m_numDescriptorsInHeap;
		uint32_t m_numFreeHandles;

		std::mutex m_allocationMutex;
	};

}