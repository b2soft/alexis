#include <Precompiled.h>

#include "CommandManager.h"

#include "Render.h"

namespace alexis
{
	CommandManager::CommandManager() :
		m_directCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
		m_computeCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
		m_copyCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY)
	{
		auto render = Render::GetInstance();
		auto device = render->GetDevice();

		m_directCommandQueue.Create(device);
		m_computeCommandQueue.Create(device);
		m_copyCommandQueue.Create(device);
	}

	CommandContext* CommandManager::CreateCommandContext(D3D12_COMMAND_LIST_TYPE type)
	{
		std::scoped_lock lock(m_allocatorMutex);

		if (m_cachedContexts.size() > 0)
		{
			CommandContext* commandContext = m_cachedContexts.front().second;
			if (IsFenceCompleted(m_cachedContexts.front().first))
			{
				m_cachedContexts.pop();
				commandContext->Reset();
				return commandContext;
			}
		}

		AllocateContext(type);

		std::wstring c;
		c = L"Allocated: " + std::to_wstring(m_commandContextPool.size()) + L"\n";

		OutputDebugString(c.c_str());
		return m_commandContextPool.at(m_commandContextPool.size() - 1).get();
	}

	void CommandManager::CacheContext(CommandContext* context, uint64_t fenceValue)
	{
		std::scoped_lock lock(m_allocatorMutex);
		m_cachedContexts.push(std::make_pair(fenceValue, context));
	}

	void CommandManager::WaitForFence(uint64_t fenceValue)
	{
		CommandQueue& producer = GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
		producer.WaitForFence(fenceValue);
	}

	void CommandManager::AllocateContext(D3D12_COMMAND_LIST_TYPE type)
	{
		auto render = Render::GetInstance();
		auto device = render->GetDevice();

		std::unique_ptr<CommandContext> context = std::make_unique<CommandContext>(type);

		ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&context->Allocator)));

		// Create Client List
		ThrowIfFailed(device->CreateCommandList(
			0,
			type,
			context->Allocator.Get(),
			nullptr,
			IID_PPV_ARGS(&context->List)));

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			context->DynamicDescriptors[i] = new DynamicDescriptorHeap(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
			context->DescriptorHeap[i] = nullptr;
		}

		m_commandContextPool.push_back(std::move(context));
	}

	CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type) :
		m_type(type),
		m_nextFenceValue((uint64_t)type << 56 | 1),
		m_lastCompletedFenceValue((uint64_t)type << 56)
	{
	}

	CommandQueue::~CommandQueue()
	{
		Destroy();
	}

	void CommandQueue::Create(ID3D12Device* device)
	{
		assert(!IsReady());

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = m_type;
		queueDesc.NodeMask = 0;

		ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fence->Signal((uint64_t)m_type << 56);

		m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	}

	void CommandQueue::Destroy()
	{
		CloseHandle(m_fenceEvent);
	}

	uint64_t CommandQueue::SignalFence()
	{
		std::scoped_lock lock(m_fenceMutex);
		m_commandQueue->Signal(m_fence.Get(), m_nextFenceValue);
		return m_nextFenceValue++;
	}

	bool CommandQueue::IsFenceCompleted(uint64_t fenceValue)
	{
		if (fenceValue > m_lastCompletedFenceValue)
		{
			m_lastCompletedFenceValue = std::max(m_lastCompletedFenceValue, m_fence->GetCompletedValue());
		}

		return fenceValue <= m_lastCompletedFenceValue;
	}

	void CommandQueue::StallForFence(uint64_t fenceValue)
	{
		CommandQueue& producer = Render::GetInstance()->GetCommandManager()->GetQueue((D3D12_COMMAND_LIST_TYPE)(fenceValue >> 56));
		m_commandQueue->Wait(producer.m_fence.Get(), fenceValue);
	}

	void CommandQueue::StallForProducer(CommandQueue& producer)
	{
		assert(producer.m_nextFenceValue > 0);
		m_commandQueue->Wait(producer.m_fence.Get(), producer.m_nextFenceValue - 1);
	}

	void CommandQueue::WaitForFence(uint64_t fenceValue)
	{
		if (IsFenceCompleted(fenceValue))
		{
			return;
		}

		{
			std::scoped_lock lock(m_eventMutex);

			m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
			WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
			m_lastCompletedFenceValue = fenceValue;
		}
	}

	uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList* list)
	{
		std::scoped_lock lock(m_fenceMutex);

		ThrowIfFailed(static_cast<ID3D12GraphicsCommandList*>(list)->Close());

		m_commandQueue->ExecuteCommandLists(1, &list);

		m_commandQueue->Signal(m_fence.Get(), m_nextFenceValue);

		return m_nextFenceValue;
	}

}

