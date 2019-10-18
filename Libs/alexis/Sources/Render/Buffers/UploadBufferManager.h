#pragma once

#include <wrl.h>
#include <d3d12.h>

#include <queue>

namespace alexis
{
	class UploadBufferManager
	{
	public:
		struct Allocation
		{
			void* Cpu;
			D3D12_GPU_VIRTUAL_ADDRESS Gpu;
			std::size_t Offset;
			ID3D12Resource* Resource;
		};

		explicit UploadBufferManager(std::size_t pageSize = 2 * 1024 * 1024); //2 MB
		~UploadBufferManager();

		std::size_t GetPageSize() const
		{
			return m_pageSize;
		}

		Allocation Allocate(std::size_t sizeInBytes, std::size_t alignment);
		void Reset();
	private:
		struct Page
		{
			Page(std::size_t sizeInBytes);
			~Page();

			bool HasSpace(std::size_t sizeInBytes, std::size_t alignment) const;

			Allocation Allocate(std::size_t sizeInBytes, std::size_t alignment);

			void Reset();

		private:
			Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

			void* m_cpuPtr;
			D3D12_GPU_VIRTUAL_ADDRESS m_gpuPtr;

			std::size_t m_pageSize;
			std::size_t m_offset;
		};

		using PagePool = std::deque<std::shared_ptr<Page>>;

		std::shared_ptr<Page> RequestPage();

		PagePool m_pagePool;
		PagePool m_availablePages;

		std::shared_ptr<Page> m_currentPage;

		std::size_t m_pageSize;
	};
}