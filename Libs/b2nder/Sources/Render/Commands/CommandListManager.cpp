#include <Precompiled.h>

#include "CommandListManager.h"

#include <Application.h>

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type)
	: m_type(type)
	, m_nextFenceValue(GetFenceValueFromType(type) | 1)
	, m_lastCompletedFenceValue(GetFenceValueFromType(type))
	, m_allocatorPool(type)
{

}

CommandQueue::~CommandQueue()
{
	Destroy();
}

CommandListManager::~CommandListManager()
{
	Destroy();
}

void CommandListManager::Create()
{
	m_device = Application::Get().GetDevice();

	m_directQueue.Create();
	m_computeQueue.Create();
	m_copyQueue.Create();
}

void CommandListManager::Destroy()
{
	m_directQueue.Destroy();
	m_computeQueue.Destroy();
	m_copyQueue.Destroy();
}

void CommandListManager::CreateCommandList(D3D12_COMMAND_LIST_TYPE type, ID3D12GraphicsCommandList** list, ID3D12CommandAllocator** allocator)
{
	assert(type != D3D12_COMMAND_LIST_TYPE_BUNDLE);
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		*allocator = m_directQueue.RequestAllocator();
		break;
	case D3D12_COMMAND_LIST_TYPE_BUNDLE:
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		*allocator = m_computeQueue.RequestAllocator();
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		*allocator = m_copyQueue.RequestAllocator();
		break;
	}

	ThrowIfFailed(m_device->CreateCommandList(1, type, *allocator, nullptr, IID_PPV_ARGS(list)));
	(*list)->SetName(L"CommandList");
}

void CommandQueue::Create() 
{
	auto device = Application::Get().GetDevice();
	assert(!IsReady());
	assert(m_allocatorPool.Size() == 0);

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = m_type;
	queueDesc.NodeMask = 0;
	device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
	m_commandQueue->SetName(L"CommandListManager::m_commandQueue");

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fence->SetName(L"CommandListManager::m_fence");
	m_fence->Signal(GetFenceValueFromType(m_type));

	m_fenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
	assert(m_fenceEventHandle != NULL);

	m_allocatorPool.Create();

	assert(IsReady());
}

void CommandQueue::Destroy()
{
	if (m_commandQueue == nullptr)
	{
		return;
	}

	m_allocatorPool.Destroy();

	CloseHandle(m_fenceEventHandle);

	m_fence->Release();
	m_fence = nullptr;
}

uint64_t CommandQueue::IncrementFence()
{
	std::lock_guard<std::mutex> lock(m_fenceMutex);
	m_commandQueue->Signal(m_fence, m_nextFenceValue);
	return m_nextFenceValue++;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
	// Avoid querying the fence value by testing against the last one seen
	// The max() is to protect against an unlikely race condition that could cause the last completed fence value to regress
	if (fenceValue > m_lastCompletedFenceValue)
	{
		m_lastCompletedFenceValue = std::max(m_lastCompletedFenceValue, m_fence->GetCompletedValue());
	}

	return fenceValue <= m_lastCompletedFenceValue;
}

namespace Graphics
{
	extern CommandListManager g_CommandManager;
}

void CommandListManager::WaitForFence(uint64_t fenceValue)
{
	CommandQueue& producer = Graphics::g_CommandManager.GetQueue(CommandQueue::GetTypeFromFenceValue(fenceValue));
	producer.WaitForFence(fenceValue);
}

void CommandQueue::StallForFence(uint64_t fenceValue)
{
	CommandQueue& producer = Graphics::g_CommandManager.GetQueue(GetTypeFromFenceValue(fenceValue));
	m_commandQueue->Wait(producer.m_fence, fenceValue);
}

void CommandQueue::StallForProducer(CommandQueue& producer)
{
	assert(producer.m_nextFenceValue > 0);
	m_commandQueue->Wait(producer.m_fence, producer.m_nextFenceValue - 1);
}

void CommandQueue::WaitForFence(uint64_t fenceValue)
{
	if (IsFenceComplete(fenceValue))
	{
		return;
	}

	// DirectX MiniEngine:
	// TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
	// wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
	// the fence can only have one event set on completion, then thread B has to wait for 
	// 100 before it knows 99 is ready.  Maybe insert sequential events?
	{
		std::lock_guard<std::mutex> lock(m_eventMutex);

		m_fence->SetEventOnCompletion(fenceValue, m_fenceEventHandle);
		WaitForSingleObject(m_fenceEventHandle, INFINITE);
		m_lastCompletedFenceValue = fenceValue;
	}
}

uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList* list)
{
	std::lock_guard<std::mutex> lock(m_fenceMutex);

	ThrowIfFailed(((ID3D12GraphicsCommandList*)list)->Close());

	// Flush command list
	m_commandQueue->ExecuteCommandLists(1, &list);

	// Signal the next fence value (with the GPU)
	m_commandQueue->Signal(m_fence, m_nextFenceValue);

	return m_nextFenceValue++;
}

ID3D12CommandAllocator* CommandQueue::RequestAllocator()
{
	uint64_t completedFence = m_fence->GetCompletedValue();

	return m_allocatorPool.RequestAllocator(completedFence);
}

void CommandQueue::DiscardAllocator(uint64_t resetFenceValue, ID3D12CommandAllocator* allocator)
{
	m_allocatorPool.DiscardAllocator(resetFenceValue, allocator);
}
