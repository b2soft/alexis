#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl.h>

#include <Utils/Singleton.h>

#include <Render/CommandManager.h>

#include <Render/RenderTarget.h>
#include <Render/Buffers/UploadBufferManager.h>

#include <Render/Descriptors/DescriptorAllocation.h>
#include <Render/Descriptors/DescriptorAllocator.h>
#include <Render/RenderTargetManager.h>

#include <Render/FrameRenderGraph.h>

using Microsoft::WRL::ComPtr;

namespace alexis
{
	class FrameRenderGraph;

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

		const RenderTarget& GetBackbufferRT() const;

		const CD3DX12_VIEWPORT& GetDefaultViewport() const;
		const CD3DX12_RECT& GetDefaultScissorRect() const;

		ID3D12Device2* GetDevice() const;

		UploadBufferManager* GetUploadBufferManager() const
		{
			return m_uploadBufferManager.get();
		}

		CommandManager* GetCommandManager() const
		{
			return m_commandManager.get();
		}

		RenderTargetManager* GetRTManager() const
		{
			return m_rtManager.get();
		}

		bool IsVSync() const;
		void SetVSync(bool vSync);
		void ToggleVSync();

		bool IsFullscreen() const;
		void SetFullscreen(bool fullscreen);
		void ToggleFullscreen();

		DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);

		void ReleaseStaleDescriptors(uint64_t fenceValue);

	private:
		void InitDevice();
		void InitPipeline();

		void UpdateRenderTargetViews();

		Microsoft::WRL::ComPtr<IDXGIAdapter4> GetHardwareAdapter(IDXGIFactory4* factory);
		static const UINT k_frameCount = 4;

		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;

		ComPtr<ID3D12Device2> m_device;
		ComPtr<IDXGISwapChain4> m_swapChain;

		TextureBuffer m_backbufferTextures[k_frameCount];
		mutable RenderTarget m_backbufferRT;

		std::unique_ptr<DescriptorAllocator> m_descriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
		std::unique_ptr<UploadBufferManager> m_uploadBufferManager;

		std::unique_ptr<FrameRenderGraph> m_frameRenderGraph;

		std::unique_ptr<CommandManager> m_commandManager;
		std::unique_ptr<RenderTargetManager> m_rtManager;

		// Sync objects
		UINT m_frameIndex{ 0 };
		UINT64 m_fenceValues[k_frameCount]{};

		HANDLE m_swapChainEvent;

		// Needed to restore window rect after transition fullscreen->windowed
		RECT m_windowRect;

		int m_windowWidth{ 1 };
		int m_windowHeight{ 1 };

		bool m_vSync{ false };
		bool m_fullscreen{ false };
		bool m_isTearingSupported{ false };
		bool m_useWarpDevice{ false };
	};

}