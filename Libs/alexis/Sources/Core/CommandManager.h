#pragma once

#include <d3d12.h>
#include <wrl.h>

#include <Utils/Singleton.h>

namespace alexis
{
	using Microsoft::WRL::ComPtr;

	class CommandManager : public alexis::Singleton<CommandManager>
	{
	public:
		void Initialize();
		void Destroy();


		ID3D12GraphicsCommandList* CreateCommandList();
		void ExecuteCommandList(ID3D12GraphicsCommandList* list);

		void WaitForFence(uint64_t fenceValue);
		bool IsFenceCompleted(uint64_t fenceValue);
		
		uint64_t SignalFence();
		void WaitForGpu();

		void Release();


	private:

		void AllocateCommand();

		struct CommandStuff
		{
			ComPtr<ID3D12CommandAllocator> allocator;
			ComPtr<ID3D12GraphicsCommandList> list;
		};

		std::vector<std::pair<uint64_t, CommandStuff>> m_commandsPool;

		friend class Render;
		ComPtr<ID3D12CommandQueue> m_directCommandQueue;
		ComPtr<ID3D12Fence> m_fence;

		HANDLE m_fenceEvent{ NULL };
		uint64_t m_lastCompletedFenceValue{ 0UL };
		uint64_t m_nextFenceValue{ 1UL };

		int currentAlloc{ 0 };
	};

}