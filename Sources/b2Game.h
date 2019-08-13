#pragma once

#include "Game.h"
#include "Window.h"

#include "Render/RenderTarget.h"
#include "Render/RootSignature.h"


#include <DirectXMath.h>

class b2Game : public Game
{
public:
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


	// Create a GPU buffer
	void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);


	// Vertex Buffer for the cube
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	// Index Buffer for the cube
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	RenderTarget m_renderTarget;

	RootSignature m_rootSignature;

	// Pipeline State Object
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	float m_fov;

	DirectX::XMMATRIX m_modelMatrix;
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projectionMatrix;

	int m_width;
	int m_height;

	bool m_contentLoaded;
};