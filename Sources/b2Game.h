#pragma once

#include "Game.h"
#include "Window.h"

#include <DirectXMath.h>

class b2Game :public Game
{
	b2Game(const std::wstring& name, int width, int height, bool vSync = false);

	virtual bool LoadContent() override;
	virtual void UnloadContent() override;

protected:
	virtual void OnUpdate(UpdateEventArgs& e) override;
	virtual void OnRender(RenderEventArgs& e) override;

	virtual void OnKeyPressed(KeyEventArgs& e) override;

	virtual void OnMouseWheel(MouseWheelEventArgs& e) override;

	virtual void OnResize(ResizeEventArgs& e) override;

private:
	// Helper functions
	// Transition a resource
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		Microsoft::WRL::ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
	
	// Clear a RTV
	void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

	// Clear Depth in DSV
	void ClearDepth(Microsoft::WRL::ComPtr< ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

	// Create a GPU buffer
	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void ResizeDepthBuffer(int width, int height);


	uint64_t m_fenceValues[Window::k_bufferCount] = {};

	// Vertex Buffer for the cube
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	// Index Buffer for the cube
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	// Depth buffer
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthBuffer;

	// Descriptor heap for depth buffer
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;

	// Root signature
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;

	// Pipeline State Object
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	float m_fov;

	DirectX::XMMATRIX m_modelMatrix;
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projectionMatrix;

	bool m_contentLoaded;
};