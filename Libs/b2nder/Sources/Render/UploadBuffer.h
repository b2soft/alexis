/**
/**
 * An UploadBuffer provides a convenient method to upload resources to the GPU.
 */
#pragma once

#include "../Utils/Defines.h"

#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <deque>

class UploadBuffer
{
public:
	// Use to upload data to the GPU
	struct Allocation
	{
		void* cpu;
		D3D12_GPU_VIRTUAL_ADDRESS gpu;
	};

	explicit UploadBuffer(size_t pageSize = _2MB);

	virtual ~UploadBuffer();

	// The maximum size of an allocation is the size of a single page
	size_t GetPageSize() const
	{
		return m_pageSize;
	}

	// Allocate memory in an Upload heap
	// An allocation must not exceed the size of a page
	// Use a memcpy or similar method to copy the buffer data to CPU pointer in the Allocation structure returned from this function
	Allocation Allocate(size_t sizeInBytes, size_t alignment);

	// Release all allocated pages
	// Must be done after cmd list is finished on the cmd queue
	void Reset();

private:
	// Single page for the allocator
	struct Page
	{
		Page(size_t sizeInBytes);
		~Page();

		// Check if the page has space
		bool HasSpace(size_t sizeInBytes, size_t alignment) const;

		// Allocate memory from the page
		// Thrwos std::bad_alloc if the alloc size larger than page or larger than free page space
		Allocation Allocate(size_t sizeInBytes, size_t alignment);

		// Reset a page for reuse
		void Reset();

	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12Resource;

		// Base pointer
		void* m_cpuPtr;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpuPtr;

		// Allocated page size
		size_t m_pageSize;

		// Current allocation offset in bytes
		size_t m_offset;
	};

	// A pool of memory pages
	using PagePool = std::deque<std::shared_ptr<Page>>;

	// Request a page from the pages pool or create new page if no pages left
	std::shared_ptr<Page> RequestPage();

	PagePool m_pagePool;
	PagePool m_availablePages;

	std::shared_ptr<Page> m_currentPage;

	// The size of page in memory
	size_t m_pageSize;
};