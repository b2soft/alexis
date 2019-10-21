#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <mutex>

#include <Render/Descriptors/DynamicDescriptorHeap.h>
#include <Render/CommandContext.h>
#include <Utils/Singleton.h>

namespace alexis
{
	using Microsoft::WRL::ComPtr;

	class CommandManager : public alexis::Singleton<CommandManager>
	{
	public:
		void Initialize();
		void Destroy();

		CommandContext* CreateCommandContext();
		uint64_t ExecuteCommandContext(CommandContext* context, bool waitForCompletion = false);
		uint64_t ExecuteCommandContexts(const std::vector<CommandContext*>& contexts, bool waitForCompletion = false);

		void WaitForFence(uint64_t fenceValue);
		bool IsFenceCompleted(uint64_t fenceValue);
		
		uint64_t SignalFence();
		void WaitForGpu();

		void Release();

		uint64_t GetLastCompletedFence() const
		{
			return m_lastCompletedFenceValue;
		}

	private:
		void AllocateContext();

		std::mutex m_allocatorMutex;

		std::vector<std::unique_ptr<CommandContext>> m_commandContextPool;
		std::queue<std::pair<uint64_t, CommandContext*>> m_cachedContexts;

		friend class Render;
		ComPtr<ID3D12CommandQueue> m_directCommandQueue;
		ComPtr<ID3D12Fence> m_fence;

		HANDLE m_fenceEvent{ NULL };
		uint64_t m_lastCompletedFenceValue{ 0UL };
		uint64_t m_nextFenceValue{ 1UL };
	};

}