#pragma once

#include <d3dx12.h>
#include <DirectXMath.h>

#include <Core/Core.h>

using Microsoft::WRL::ComPtr;

class SampleApp : public alexis::IGame
{
public:
	SampleApp();

	struct SceneConstantBuffer
	{
		DirectX::XMFLOAT4 offset;
	};

	virtual bool LoadContent();
	virtual void UnloadContent();

	virtual void OnUpdate(float dt) override;
	virtual void OnRender() override;

private:
	static const UINT k_frameCount = 2;

	// Temporary texture stuff
	static const UINT k_textureSize = 256;
	static const UINT k_texturePixelSize = 4;

	// Pipeline objects
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;
	ComPtr<ID3D12CommandAllocator> m_bundleAllocator;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_srvHeap;
	ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12GraphicsCommandList> m_bundle;

	float m_aspectRatio{ 1.0f };

	// Application resources
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	ComPtr<ID3D12Resource> m_constantBuffer;
	UINT8* m_pCbvDataBegin{ nullptr };
	SceneConstantBuffer m_constantBufferData;
	ComPtr<ID3D12Resource> m_texture;

	float m_timeCount{ 0.f };
	int m_frameCount{ 0 };
	float m_fps{ 0.f };

	void LoadPipeline();
	void LoadAssets();
	std::vector<UINT8> GenerateTextureData();
	void PopulateCommandList();
};