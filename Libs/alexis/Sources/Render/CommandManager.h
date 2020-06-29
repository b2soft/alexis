#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <mutex>
#include <queue>

#include <Render/CommandContext.h>
#include <Utils/Singleton.h>

namespace alexis
{
	using Microsoft::WRL::ComPtr;


	class CommandQueue
	{
		friend class CommandManager;
		friend class CommandContext;

	public:
		CommandQueue(D3D12_COMMAND_LIST_TYPE type);
		~CommandQueue();

		void Create(ID3D12Device* device);
		void Destroy();

		bool IsReady()
		{
			return m_commandQueue != nullptr;
		}

		uint64_t SignalFence();
		bool IsFenceCompleted(uint64_t fenceValue);

		void StallForFence(uint64_t fenceValue);
		void StallForProducer(CommandQueue& producer);

		void WaitForFence(uint64_t fenceValue);
		void WaitForGpu()
		{
			WaitForFence(SignalFence());
		}

		ID3D12CommandQueue* GetCommandQueue()
		{
			return m_commandQueue.Get();
		}


	private:
		uint64_t ExecuteCommandList(ID3D12CommandList* list);

		ComPtr<ID3D12CommandQueue> m_commandQueue;

		std::mutex m_fenceMutex;
		std::mutex m_eventMutex;

		const D3D12_COMMAND_LIST_TYPE m_type;

		ComPtr<ID3D12Fence> m_fence;
		HANDLE m_fenceEvent{ NULL };
		uint64_t m_lastCompletedFenceValue{ 0UL };
		uint64_t m_nextFenceValue{ 1UL };
	};

	class CommandManager
	{
	public:
		CommandManager();

		CommandQueue& GetGraphicsQueue()
		{
			return m_directCommandQueue;
		}

		CommandQueue& GetComputeQueue()
		{
			return m_computeCommandQueue;
		}

		CommandQueue& GetCopyQueue()
		{
			return m_copyCommandQueue;
		}

		CommandQueue& GetQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT)
		{
			switch (type)
			{
			case D3D12_COMMAND_LIST_TYPE_COMPUTE:
				return m_computeCommandQueue;
			case D3D12_COMMAND_LIST_TYPE_COPY:
				return m_copyCommandQueue;
			default:
				return m_directCommandQueue;
			}

			return m_directCommandQueue;
		}

		bool IsFenceCompleted(uint64_t fenceValue)
		{
			return GetQueue(D3D12_COMMAND_LIST_TYPE(fenceValue >> 56)).IsFenceCompleted(fenceValue);
		}

		void WaitForFence(uint64_t fenceValue);
		void WaitForGpu()
		{
			m_directCommandQueue.WaitForGpu();
			m_computeCommandQueue.WaitForGpu();
			m_copyCommandQueue.WaitForGpu();
		}

		CommandContext* CreateCommandContext(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT);
		void CacheContext(CommandContext* context, uint64_t fenceValue);

	private:
		void AllocateContext(D3D12_COMMAND_LIST_TYPE type);

		std::mutex m_allocatorMutex;

		std::vector<std::unique_ptr<CommandContext>> m_commandContextPool;
		std::queue<std::pair<uint64_t, CommandContext*>> m_cachedContexts;

		friend class Render;
		CommandQueue m_directCommandQueue;
		CommandQueue m_computeCommandQueue;
		CommandQueue m_copyCommandQueue;
	};

}