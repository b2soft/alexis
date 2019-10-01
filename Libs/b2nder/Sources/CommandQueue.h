#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <atomic>
#include <cstdint>
#include <queue>
#include <condition_variable>

#include "ThreadSafeQueue.h"

class CommandList;

class CommandQueue
{
public:
	CommandQueue(D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandQueue();

	// Get an available list from the command queue
	std::shared_ptr<CommandList> GetCommandList();

	// Execute a command list
	// Returns the fence value to wait for for this command list
	uint64_t ExecuteCommandList(std::shared_ptr<CommandList> commandList);
	uint64_t ExecuteCommandLists(const std::vector<std::shared_ptr<CommandList>>& commandLists);

	uint64_t Signal();
	bool IsFenceCompleted(uint64_t fenceValue);
	void WaitForFenceValue(uint64_t fenceValue);
	void Flush();

	// Wait for other comm queue to finish
	void Wait(const CommandQueue& other);

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

private:
	// Free any command lists that are finished processing on the command queue
	void ProcessInFlightCommandLists();

	// Keep track of command allocators that are "in-flight"
	// The first member is the fence value to wait for, the second is the a shared pointer to the "in-flight" command list
	using CommandListEntry = std::tuple<uint64_t, std::shared_ptr<CommandList>>;

	D3D12_COMMAND_LIST_TYPE						m_commandListType;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>	m_d3d12CommandQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence>			m_d3d12Fence;
	uint64_t									m_fenceValue;

	ThreadSafeQueue<CommandListEntry>			m_inFlightCommandLists;

	ThreadSafeQueue<std::shared_ptr<CommandList>>			m_availableCommandLists;

	// A thread to process in-flight command lists
	std::thread m_processInFlightCommandListsThread;
	std::atomic_bool m_processInFlightCommandLists;
	std::mutex m_processInFlightCommandListsThreadMutex;
	std::condition_variable m_processInFlightCommandListsThreadCV;
};