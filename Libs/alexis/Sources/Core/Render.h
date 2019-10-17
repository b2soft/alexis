#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl.h>

#include <Utils/Singleton.h>
#include <Render/Descriptors/DescriptorAllocation.h>
#include <Render/Descriptors/DescriptorAllocator.h>

using Microsoft::WRL::ComPtr;

namespace alexis
{
	class DescriptorAllocator;

	class Render : public alexis::Singleton<Render>
	{
	public:
		void Initialize(int width, int height);
		void Destroy();

		// Flush allocators and lists
		void BeginRender();

		// Present to the screen
		void Present();

		void OnResize(int width, int height);

		ID3D12Resource* GetCurrentBackBufferResource() const;
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;

		const CD3DX12_VIEWPORT& GetDefaultViewport() const
		{
			return m_viewport;
		}

		const CD3DX12_RECT& GetDefaultScrissorRect() const
		{
			return m_scissorRect;
		}

		ID3D12Device* GetDevice() const
		{
			return m_device.Get();
		}

		DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);

	private:
		void InitDevice();
		void InitPipeline();

		void ReleaseStaleDescriptors(uint64_t fenceValue);

		Microsoft::WRL::ComPtr<IDXGIAdapter4> GetHardwareAdapter(IDXGIFactory4* factory);
		static const UINT k_frameCount = 2;

		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;

		ComPtr<ID3D12Device> m_device;
		ComPtr<IDXGISwapChain4> m_swapChain;
		ComPtr<ID3D12Resource> m_backbufferTextures[k_frameCount];
		ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		UINT m_rtvDescriptorSize{ 0 };

		std::unique_ptr<DescriptorAllocator> m_descriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		// Sync objects
		UINT m_frameIndex{ 0 };
		UINT64 m_fenceValues[k_frameCount]{};

		HANDLE m_swapChainEvent;
		int m_windowWidth{ 1 };
		int m_windowHeight{ 1 };
		bool m_vSync{ false };
		bool m_isTearingSupported{ false };// TODO: faked, need to obtain real value
		bool m_useWarpDevice{ false };
	};

}