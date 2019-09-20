#include <Precompiled.h>

#include <new>

#include "UploadBuffer.h"

#include "../Application.h"
#include "../Helpers.h"

UploadBuffer::UploadBuffer(size_t pageSize /*= _2MB*/)
	: m_pageSize(pageSize)
{
}

UploadBuffer::~UploadBuffer()
{
}

UploadBuffer::Allocation UploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (sizeInBytes > m_pageSize)
	{
		throw std::bad_alloc();
	}

	// If there is no current page, or no space in the current page, request a new
	if (!m_currentPage || !m_currentPage->HasSpace(sizeInBytes, alignment))
	{
		m_currentPage = RequestPage();
	}

	return m_currentPage->Allocate(sizeInBytes, alignment);
}

void UploadBuffer::Reset()
{
	m_currentPage = nullptr;

	// Reset all available pages
	m_availablePages = m_pagePool;

	for (auto& page : m_availablePages)
	{
		// Reset the page for new allocations
		page->Reset();
	}
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
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

UploadBuffer::Page::Page(size_t sizeInBytes)
	: m_pageSize(sizeInBytes)
	, m_offset(0)
	, m_cpuPtr(nullptr)
	, m_gpuPtr(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
	auto device = Application::Get().GetDevice();

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_pageSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_d3d12Resource)
	));

	m_gpuPtr = m_d3d12Resource->GetGPUVirtualAddress();
	m_d3d12Resource->Map(0, nullptr, &m_cpuPtr);
}

UploadBuffer::Page::~Page()
{
	m_d3d12Resource->Unmap(0, nullptr);
	m_cpuPtr = nullptr;
	m_gpuPtr = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
	size_t alignedSize = Math::AlignUp(sizeInBytes, alignment);
	size_t alignedOffset = Math::AlignUp(m_offset, alignment);

	return alignedOffset + alignedSize <= m_pageSize;
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (!HasSpace(sizeInBytes, alignment))
	{
		throw std::bad_alloc();
	}

	size_t alignedSize = Math::AlignUp(sizeInBytes, alignment);
	m_offset = Math::AlignUp(m_offset, alignment);

	Allocation allocation;
	allocation.cpu = static_cast<uint8_t*>(m_cpuPtr) + m_offset;
	allocation.gpu = m_gpuPtr + m_offset;

	m_offset += alignedSize;

	return allocation;
}

void UploadBuffer::Page::Reset()
{
	m_offset = 0;
}
