#pragma once

#include <d3dx12.h>
#include <DirectXMath.h>
#include <wrl.h>

#include <Utils/Singleton.h>

using Microsoft::WRL::ComPtr;

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

	ComPtr<ID3D12GraphicsCommandList> GetGraphicsCommandList();

	ID3D12Resource* GetCurrentBackBufferResource() const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferRTV() const;

	const CD3DX12_VIEWPORT& GetViewport() const
	{
		return m_viewport;
	}

	const CD3DX12_RECT& GetScrissorRect() const
	{
		return m_scissorRect;
	}


private:
	void MoveToNextFrame();
	void WaitForGpu();

	void InitDevice();
	void InitPipeline();

	Microsoft::WRL::ComPtr<IDXGIAdapter4> GetHardwareAdapter(IDXGIFactory4* factory);
	static const UINT k_frameCount = 2;

	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	ComPtr<ID3D12Device> m_device;
	ComPtr<IDXGISwapChain4> m_swapChain;
	ComPtr<ID3D12Resource> m_backbufferTextures[k_frameCount];
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12CommandAllocator> m_clientCommandAllocators[k_frameCount];
	ComPtr<ID3D12GraphicsCommandList> m_clientCommandList;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	UINT m_rtvDescriptorSize{ 0 };

	// Sync objects
	UINT m_frameIndex{ 0 };
	HANDLE m_fenceEvent{ NULL };
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValues[k_frameCount]{};

	int m_windowWidth{ 1 };
	int m_windowHeight{ 1 };
	bool m_vSync{ false };
	bool m_isTearingSupported{ false };// TODO: faked, need to obtain real value
	bool m_useWarpDevice{ false };
};