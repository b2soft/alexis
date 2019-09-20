#include <Precompiled.h>

#include "../Application.h"
#include "DescriptorAllocatorPage.h"

#include "DescriptorAllocation.h"

DescriptorAllocation::DescriptorAllocation()
	: m_descriptor{ 0 }
	, m_numHandles(0)
	, m_descriptorSize(0)
	, m_page(nullptr)
{
}

DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t numHandles, uint32_t descriptorSize, std::shared_ptr<DescriptorAllocatorPage> page)
	: m_descriptor(descriptor)
	, m_numHandles(numHandles)
	, m_descriptorSize(descriptorSize)
	, m_page(page)
{
}

DescriptorAllocation::~DescriptorAllocation()
{
	Free();
}

DescriptorAllocation::DescriptorAllocation(DescriptorAllocation&& allocation)
	: m_descriptor(allocation.m_descriptor)
	, m_numHandles(allocation.m_numHandles)
	, m_descriptorSize(allocation.m_descriptorSize)
	, m_page(std::move(allocation.m_page))
{
	allocation.m_descriptor.ptr = 0;
	allocation.m_numHandles = 0;
	allocation.m_descriptorSize = 0;
}

DescriptorAllocation& DescriptorAllocation::operator=(DescriptorAllocation&& allocation)
{
	Free();

	m_descriptor = allocation.m_descriptor;
	m_numHandles = allocation.m_numHandles;
	m_descriptorSize = allocation.m_descriptorSize;
	m_page = std::move(allocation.m_page);

	allocation.m_descriptor.ptr = 0;
	allocation.m_numHandles = 0;
	allocation.m_descriptorSize = 0;

	return *this;
}


bool DescriptorAllocation::IsNull() const
{
	return m_descriptor.ptr == 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetDescriptorHandle(uint32_t offset /*= 0*/) const
{
	assert(offset < m_numHandles);
	return { m_descriptor.ptr + (m_descriptorSize * offset) };
}

uint32_t DescriptorAllocation::GetNumHandles() const
{
	return m_numHandles;
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocation::GetDescriptorAllocatorPage() const
{
	return m_page;
}

void DescriptorAllocation::Free()
{
	if (!IsNull() && m_page)
	{
		m_page->Free(std::move(*this), Application::GetFrameCount());

		m_descriptor.ptr = 0;
		m_numHandles = 0;
		m_descriptorSize = 0;
		m_page.reset();
	}
}