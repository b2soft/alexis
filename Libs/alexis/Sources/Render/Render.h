#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl.h>

#include <Utils/Singleton.h>

#include <Render/CommandManager.h>
#include <Render/Buffers/UploadBufferManager.h>
#include <Render/FrameRenderGraph.h>
#include <Render/RenderTarget.h>
#include <Render/RenderTargetManager.h>

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

		struct DescriptorRecord
		{
			D3D12_CPU_DESCRIPTOR_HANDLE CpuPtr;
			D3D12_GPU_DESCRIPTOR_HANDLE GpuPtr;
			std::size_t OffsetInHeap;
		};

		DescriptorRecord AllocateSRV(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC desc);
		DescriptorRecord AllocateRTV(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC desc);
		DescriptorRecord AllocateDSV(ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC desc);

		void UpdateSRV(ID3D12Resource* resource, D3D12_SHADER_RESOURCE_VIEW_DESC desc, CD3DX12_CPU_DESCRIPTOR_HANDLE handle);
		void UpdateRTV(ID3D12Resource* resource, D3D12_RENDER_TARGET_VIEW_DESC desc, CD3DX12_CPU_DESCRIPTOR_HANDLE handle);
		void UpdateDSV(ID3D12Resource* resource, D3D12_DEPTH_STENCIL_VIEW_DESC desc, CD3DX12_CPU_DESCRIPTOR_HANDLE handle);

		ID3D12DescriptorHeap* GetSrvUavHeap() const;
		ID3D12DescriptorHeap* GetRtvHeap() const;
		ID3D12DescriptorHeap* GetDsvHeap() const;

	private:
		void InitDevice();
		void InitPipeline();

		void UpdateRenderTargetViews();

		Microsoft::WRL::ComPtr<IDXGIAdapter4> GetHardwareAdapter(IDXGIFactory4* factory);
		static const UINT k_frameCount = 3;

		CD3DX12_VIEWPORT m_viewport;
		CD3DX12_RECT m_scissorRect;

		ComPtr<ID3D12Device2> m_device;
		ComPtr<IDXGISwapChain4> m_swapChain;

		std::array<RenderTarget, k_frameCount> m_backbuffers;

		std::unique_ptr<UploadBufferManager> m_uploadBufferManager;

		std::unique_ptr<FrameRenderGraph> m_frameRenderGraph;

		std::unique_ptr<CommandManager> m_commandManager;
		std::unique_ptr<RenderTargetManager> m_rtManager;

		// global simple heaps
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;
		std::size_t m_allocatedSrvUavs{ 0 };
		UINT m_incrementSrvUav{ 0 };

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
		std::size_t m_allocatedRtvs{ 0 };
		UINT m_incrementRtv{ 0 };

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
		std::size_t m_allocatedDsvs{ 0 };
		UINT m_incrementDsv{ 0 };


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