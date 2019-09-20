#include <Precompiled.h>

#include "DescriptorAllocator.h"
#include "DescriptorAllocatorPage.h"

DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap /*= 256*/)
	: m_heapType(type)
	, m_numDescriptorsPerHeap(numDescriptorsPerHeap)
{
}

DescriptorAllocator::~DescriptorAllocator()
{
}

DescriptorAllocation DescriptorAllocator::Allocate(uint32_t numDescriptors /*= 1*/)
{
	std::lock_guard<std::mutex> lock(m_allocationMutex);

	DescriptorAllocation allocation;

	for (auto it = m_availableHeaps.begin(); it != m_availableHeaps.end(); ++it)
	{
		auto allocatorPage = m_heapPool[*it];

		allocation = allocatorPage->Allocate(numDescriptors);

		if (allocatorPage->NumFreeHandles() == 0)
		{
			it = m_availableHeaps.erase(it);
		}

		if (!allocation.IsNull())
		{
			break;
		}
	}

	// No available heap satisfy requested number of descriptors
	if (allocation.IsNull())
	{
		m_numDescriptorsPerHeap = std::max(m_numDescriptorsPerHeap, numDescriptors);
		auto newPage = CreateAllocatorPage();

		allocation = newPage->Allocate(numDescriptors);
	}

	return allocation;
}

void DescriptorAllocator::ReleaseStaleDescriptors(uint64_t frameNumber)
{
	std::lock_guard<std::mutex> lock(m_allocationMutex);

	for (size_t i = 0; i < m_heapPool.size(); ++i)
	{
		auto page = m_heapPool[i];
		page->ReleaseStaleDescriptors(frameNumber);

		if (page->NumFreeHandles() > 0)
		{
			m_availableHeaps.insert(i);
		}
	}
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
{
	auto newPage = std::make_shared<DescriptorAllocatorPage>(m_heapType, m_numDescriptorsPerHeap);

	m_heapPool.emplace_back(newPage);
	m_availableHeaps.insert(m_heapPool.size() - 1);

	return newPage;
}
