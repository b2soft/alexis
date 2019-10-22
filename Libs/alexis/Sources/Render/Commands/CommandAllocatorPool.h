#pragma once

#include <mutex>
#include <queue>
#include <vector>
#include <cstdint>

class CommandAllocatorPool
{
public:
	CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE type);
	~CommandAllocatorPool();

	void Create();
	void Destroy();

	ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
	void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator);

	std::size_t Size();

private:
	const D3D12_COMMAND_LIST_TYPE m_commandListType;

	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	std::vector<ID3D12CommandAllocator*> m_allocatorPool;

	using AllocatorEntry = std::pair<uint64_t, ID3D12CommandAllocator*>;
	std::queue<AllocatorEntry> m_availableAllocators;
	std::mutex m_allocatorMutex;
};