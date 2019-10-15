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
	}

	void CommandManager::Destroy()
	{
		CloseHandle(m_fenceEvent);
	}

	ID3D12GraphicsCommandList* CommandManager::CreateCommandList()
	{
		for (auto& stuff : m_commandsPool)
		{
			if (IsFenceCompleted(stuff.first))
			{
				stuff.second.allocator->Reset();
				stuff.second.list->Reset(stuff.second.allocator.Get(), nullptr);
				return stuff.second.list.Get();
			}
		}

		AllocateCommand();

		std::wstring c;
		c = L"Allocated: " + std::to_wstring(m_commandsPool.size()) + L"\n";

		OutputDebugString(c.c_str());
		return m_commandsPool.at(m_commandsPool.size() - 1).second.list.Get();
	}

	void CommandManager::ExecuteCommandList(ID3D12GraphicsCommandList* list)
	{
		list->Close();

		ID3D12CommandList* ppCommandLists[] = { list };
		m_directCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

		m_directCommandQueue->Signal(m_fence.Get(), m_nextFenceValue);

		for (auto& stuff : m_commandsPool)
		{
			if (stuff.second.list.Get() == list)
			{
				stuff.first = m_nextFenceValue;
			}
		}

		m_nextFenceValue++;
	}

	void CommandManager::WaitForFence(uint64_t fenceValue)
	{
		if (IsFenceCompleted(fenceValue))
		{
			return;
		}

		m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
		WaitForSingleObject(m_fenceEvent, INFINITE);
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

	void CommandManager::AllocateCommand()
	{
		auto render = Render::GetInstance();
		auto device = render->GetDevice();

		CommandStuff stuff;

		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&stuff.allocator)));

		// Create Client List
		ThrowIfFailed(device->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			stuff.allocator.Get(),
			nullptr,
			IID_PPV_ARGS(&stuff.list)));

		m_commandsPool.push_back(std::make_pair(m_lastCompletedFenceValue, std::move(stuff)));
	}

}

