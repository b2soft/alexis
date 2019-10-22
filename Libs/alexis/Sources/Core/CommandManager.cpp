#include <Precompiled.h>

#include "CommandManager.h"

#include "Render.h"

namespace alexis
{
	void CommandManager::Initialize()
	{
		auto render = Render::GetInstance();
		auto device = render->GetDevice();
		// Create Command Queue
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc = {};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

			ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_directCommandQueue)));
		}

		ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	void CommandManager::Destroy()
	{
		CloseHandle(m_fenceEvent);
	}

	CommandContext* CommandManager::CreateCommandContext()
	{
		std::lock_guard<std::mutex> lock(m_allocatorMutex);

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

		AllocateContext();

		std::wstring c;
		c = L"Allocated: " + std::to_wstring(m_commandContextPool.size()) + L"\n";

		OutputDebugString(c.c_str());
		return m_commandContextPool.at(m_commandContextPool.size() - 1).get();
	}

	uint64_t CommandManager::ExecuteCommandContext(CommandContext* context, bool waitForCompletion)
	{
		context->List.Get()->Close();

		ID3D12CommandList* ppCommandLists[] = { context->List.Get() };
		m_directCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		uint64_t fenceValue = m_nextFenceValue;

		m_directCommandQueue->Signal(m_fence.Get(), fenceValue);

		{
			std::lock_guard<std::mutex> lock(m_allocatorMutex);
			m_cachedContexts.push(std::make_pair(fenceValue, context));
		}

		if (waitForCompletion)
		{
			WaitForFence(fenceValue);
		}

		m_nextFenceValue++;

		return fenceValue;
	}

	uint64_t CommandManager::ExecuteCommandContexts(const std::vector<CommandContext*>& contexts, bool waitForCompletion /*= false*/)
	{
		std::vector<ID3D12CommandList*> lists;
		lists.reserve(contexts.size());

		for (auto context : contexts)
		{
			context->List.Get()->Close();

			lists.push_back(context->List.Get());
		}

		m_directCommandQueue->ExecuteCommandLists(lists.size(), lists.data());

		uint64_t fenceValue = m_nextFenceValue;

		m_directCommandQueue->Signal(m_fence.Get(), fenceValue);

		{
			std::lock_guard<std::mutex> lock(m_allocatorMutex);

			for (auto context : contexts)
			{
				m_cachedContexts.push(std::make_pair(fenceValue, context));
			}
		}

		if (waitForCompletion)
		{
			WaitForFence(fenceValue);
		}

		m_nextFenceValue++;

		return fenceValue;
	}

	void CommandManager::WaitForFence(uint64_t fenceValue)
	{
		if (IsFenceCompleted(fenceValue))
		{
			return;
		}

		m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
		m_lastCompletedFenceValue = fenceValue;
	}

	bool CommandManager::IsFenceCompleted(uint64_t fenceValue)
	{
		if (fenceValue > m_lastCompletedFenceValue)
		{
			m_lastCompletedFenceValue = std::max(m_lastCompletedFenceValue, m_fence->GetCompletedValue());
		}

		return fenceValue <= m_lastCompletedFenceValue;
	}

	uint64_t CommandManager::SignalFence()
	{
		m_directCommandQueue->Signal(m_fence.Get(), m_nextFenceValue);
		return m_nextFenceValue++;
	}

	void CommandManager::WaitForGpu()
	{
		WaitForFence(SignalFence());
	}

	void CommandManager::Release()
	{
	}

	void CommandManager::AllocateContext()
	{
		auto render = Render::GetInstance();
		auto device = render->GetDevice();

		std::unique_ptr<CommandContext> context = std::make_unique<CommandContext>();

		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&context->Allocator)));

		// Create Client List
		ThrowIfFailed(device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
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

}

