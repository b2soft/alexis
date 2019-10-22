#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <cstdint>
#include "CommandAllocatorPool.h"

class CommandQueue
{
	friend class CommandListManager;
	friend class CommandContext;

public:
	CommandQueue(D3D12_COMMAND_LIST_TYPE type);
	~CommandQueue();

	void Create();
	void Destroy();

	inline bool IsReady()
	{
		return true;
	}

	uint64_t IncrementFence();
	bool IsFenceComplete(uint64_t fenceValue);

	void StallForFence(uint64_t fenceValue);
	void StallForProducer(CommandQueue& producer);

	void WaitForFence(uint64_t fenceValue);
	void WaitForIdle()
	{
		WaitForFence(IncrementFence());
	}

	ID3D12CommandQueue* GetD3D12CommandQueue()
	{
		return m_commandQueue;
	}

	uint64_t GetNextFenceValue()
	{
		return m_nextFenceValue;
	}
	
	inline static constexpr D3D12_COMMAND_LIST_TYPE GetTypeFromFenceValue(uint64_t fenceValue)
	{
		return (D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56);
	}

	inline static constexpr uint64_t GetFenceValueFromType(D3D12_COMMAND_LIST_TYPE type)
	{
		return type << 56;
	}

private:
	uint64_t ExecuteCommandList(ID3D12CommandList* list);
	ID3D12CommandAllocator* RequestAllocator();
	void DiscardAllocator(uint64_t resetFenceValue, ID3D12CommandAllocator* allocator);


	ID3D12CommandQueue* m_commandQueue{ nullptr };

	const D3D12_COMMAND_LIST_TYPE m_type;

	CommandAllocatorPool m_allocatorPool;
	std::mutex m_fenceMutex;
	std::mutex m_eventMutex;

	// Lifetime of these objects is managed by the descriptor cache
	ID3D12Fence* m_fence{ nullptr };
	uint64_t m_nextFenceValue;
	uint64_t m_lastCompletedFenceValue;
	HANDLE m_fenceEventHandle;
};

class CommandListManager
{
	friend class CommandContext;

public:
	~CommandListManager();

	void Create();
	void Destroy();

	CommandQueue& GetGraphicsQueue()
	{
		return m_directQueue;
	}

	CommandQueue& GetComputeQueue()
	{
		return m_computeQueue;
	}

	CommandQueue& GetCopyQueue()
	{
		return m_copyQueue;
	}

	CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT)
	{
		switch (type)
		{
		case D3D12_COMMAND_LIST_TYPE_COMPUTE:
			return m_computeQueue;
		case D3D12_COMMAND_LIST_TYPE_COPY:
			return m_copyQueue;
		default:
			return m_directQueue;
		}
	}

	ID3D12CommandQueue* GetD3D12CommandQueue()
	{
		return m_directQueue.GetD3D12CommandQueue();
	}

	void CreateCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** list, ID3D12CommandAllocator** allocator);

	// Test to see if a fence has already been reached
	bool IsFenceComplete(uint64_t fenceValue)
	{
		return GetQueue(CommandQueue::GetTypeFromFenceValue(fenceValue)).IsFenceComplete(fenceValue);
	}

	// CPU wait for fence
	void WaitForFence(uint64_t fenceValue);

	// CPU wait for all command queues to be empty (GPU will idle then)
	void WaitForIdleGPU()
	{
		m_directQueue.WaitForIdle();
		m_computeQueue.WaitForIdle();
		m_copyQueue.WaitForIdle();
	}


private:
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;

	CommandQueue m_directQueue{ D3D12_COMMAND_LIST_TYPE_DIRECT };
	CommandQueue m_computeQueue{ D3D12_COMMAND_LIST_TYPE_COMPUTE };
	CommandQueue m_copyQueue{ D3D12_COMMAND_LIST_TYPE_COPY };
};
