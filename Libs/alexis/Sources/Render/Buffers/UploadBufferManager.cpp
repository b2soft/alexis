#include <Precompiled.h>

#include "UploadBufferManager.h"

#include <new>

#include <Core/Render.h>

namespace alexis
{

	UploadBufferManager::UploadBufferManager(std::size_t pageSize /*= 2 * 1024 * 1024*/) :
		m_pageSize(pageSize)
	{

	}

	UploadBufferManager::~UploadBufferManager()
	{
	}

	alexis::UploadBufferManager::Allocation UploadBufferManager::Allocate(std::size_t sizeInBytes, std::size_t alignment)
	{
		if (sizeInBytes > m_pageSize)
		{
			throw std::bad_alloc();
		}

		if (!m_currentPage || !m_currentPage->HasSpace(sizeInBytes, alignment))
		{
			m_currentPage = RequestPage();
		}

		return m_currentPage->Allocate(sizeInBytes, alignment);
	}

	void UploadBufferManager::Reset()
	{
		m_currentPage = nullptr;
		// Reset all available pages.
		m_availablePages = m_pagePool;

		for (auto page : m_availablePages)
		{
			// Reset the page for new allocations.
			page->Reset();
		}
	}

	std::shared_ptr<alexis::UploadBufferManager::Page> UploadBufferManager::RequestPage()
	{
		std::shared_ptr<Page> page;

		if (!m_availablePages.empty())
		{
			page = m_availablePages.front();
			m_availablePages.pop_front();
		}
		else
		{
			page = std::make_shared<Page>(m_pageSize);
			m_pagePool.push_back(page);
		}

		return page;
	}

	UploadBufferManager::Page::Page(std::size_t sizeInBytes) :
		m_pageSize(sizeInBytes),
		m_offset(0),
		m_cpuPtr(nullptr),
		m_gpuPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
	{
		auto device = Render::GetInstance()->GetDevice();

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_pageSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_resource)
		));

		std::wstring name = L"Upload buffer " + std::to_wstring(m_offset);
		SetName(m_resource.Get(), name.c_str());

		m_gpuPtr = m_resource->GetGPUVirtualAddress();
		m_resource->Map(0, nullptr, &m_cpuPtr);
	}

	UploadBufferManager::Page::~Page()
	{
		m_resource->Unmap(0, nullptr);
		m_cpuPtr = nullptr;
		m_gpuPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
	}

	bool UploadBufferManager::Page::HasSpace(std::size_t sizeInBytes, std::size_t alignment) const
	{
		size_t alignedSize = Math::AlignUp(sizeInBytes, alignment);
		size_t alignedOffset = Math::AlignUp(m_offset, alignment);

		return alignedOffset + alignedSize <= m_pageSize;
	}

	UploadBufferManager::Allocation UploadBufferManager::Page::Allocate(std::size_t sizeInBytes, std::size_t alignment)
	{
		if (!HasSpace(sizeInBytes, alignment))
		{
			// Can't allocate space from page.
			throw std::bad_alloc();
		}

		size_t alignedSize = Math::AlignUp(sizeInBytes, alignment);
		m_offset = Math::AlignUp(m_offset, alignment);

		Allocation allocation;
		allocation.Cpu = static_cast<uint8_t*>(m_cpuPtr) + m_offset;
		allocation.Gpu = m_gpuPtr + m_offset;
		allocation.Resource = m_resource.Get();
		allocation.Offset = m_offset;

		m_offset += alignedSize;

		return allocation;
	}

	void UploadBufferManager::Page::Reset()
	{
		m_offset = 0;
	}

}