#pragma once

#include <d3dx12.h>
#include <DirectXMath.h>

#include <Core_Old.h>

using Microsoft::WRL::ComPtr;

class SampleApp : public alexis::Core_Old
{
public:
	SampleApp(UINT width, UINT height, std::wstring name);

	struct SceneConstantBuffer
	{
		DirectX::XMFLOAT4 offset;
	};

	virtual void OnInit() override;
	virtual void OnUpdate() override;
	virtual void OnRender() override;
	virtual void OnDestroy() override;

private:
	static const UINT k_frameCount = 2;

	// Temporary texture stuff
	static const UINT k_textureSize = 256;
	static const UINT k_texturePixelSize = 4;

	// Pipeline objects
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain4> m_swapChain;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Resource> m_renderTargets[k_frameCount];
	ComPtr<ID3D12CommandAllocator> m_commandAllocators[k_frameCount];
	ComPtr<ID3D12CommandAllocator> m_bundleAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12GraphicsCommandList> m_bundle;
	UINT m_rtvDescriptorSize{ 0 };

	// Application resources
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_constantBuffer;
	UINT8* m_pCbvDataBegin{ nullptr };
	SceneConstantBuffer m_constantBufferData;
	ComPtr<ID3D12Resource> m_texture;

	// Sync objects
	UINT m_frameIndex{ 0 };
	HANDLE m_fenceEvent{ NULL };
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValues[k_frameCount];

	void LoadPipeline();
	void LoadAssets();
	std::vector<UINT8> GenerateTextureData();
	void PopulateCommandList();
	void MoveToNextFrame();
	void WaitForGpu();
};