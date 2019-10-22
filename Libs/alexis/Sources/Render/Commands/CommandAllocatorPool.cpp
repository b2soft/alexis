#include <Precompiled.h>

#include "CommandAllocatorPool.h"

#include <Application.h>

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type)
	: m_commandListType(type)
{
}

CommandAllocatorPool::~CommandAllocatorPool()
{
	Destroy();
}

void CommandAllocatorPool::Create()
{
	m_device = Application::Get().GetDevice();
}

void CommandAllocatorPool::Destroy()
{
	for (auto& allocator : m_allocatorPool)
	{
		allocator->Release();
	}

	m_allocatorPool.clear();
}

ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(uint64_t completedFenceValue)
{
	std::lock_guard<std::mutex> lock(m_allocatorMutex);

	ID3D12CommandAllocator* allocator = nullptr;

	if (!m_availableAllocators.empty())
	{
		AllocatorEntry& allocatorEntry = m_availableAllocators.front();

		if (allocatorEntry.first <= completedFenceValue)
		{
			allocator = allocatorEntry.second;
			ThrowIfFailed(allocator->Reset());
			m_availableAllocators.pop();
		}
	}

	// If no allocator in pool - create new
	if (!allocator)
	{
		ThrowIfFailed(m_device->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&allocator)));
		wchar_t allocatorName[32];
		swprintf(allocatorName, L"CommandAllator %zu", m_allocatorPool.size());
		allocator->SetName(allocatorName);
		m_allocatorPool.push_back(allocator);
	}

	return allocator;
}

void CommandAllocatorPool::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator)
{
	std::lock_guard<std::mutex> lock(m_allocatorMutex);

	// That fence value indicates we are free to reset the allocator
	m_availableAllocators.push(std::make_pair(fenceValue, allocator));
}

std::size_t CommandAllocatorPool::Size()
{
	return m_allocatorPool.size();
}
