#pragma once

#include <d3dx12.h>
#include <DirectXMath.h>

#include <Core/Core.h>
#include <Render/RootSignature.h>
#include <Render/Buffers/GpuBuffer.h>
#include <Render/Mesh.h>

using Microsoft::WRL::ComPtr;

class SampleApp : public alexis::IGame
{
public:
	SampleApp();

	struct SceneConstantBuffer
	{
		DirectX::XMFLOAT4 offset{ 1.0f, 0.0f, 1.0f, 1.0f };
	};

	virtual bool Initialize() override;

	virtual bool LoadContent();
	virtual void UnloadContent();

	virtual void OnUpdate(float dt) override;
	virtual void OnRender() override;

	virtual void Destroy() override;

private:
	static const UINT k_frameCount = 2;

	// Pipeline objects
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	ComPtr<ID3D12PipelineState> m_pipelineState;

	alexis::TextureBuffer m_checkerTexture;
	alexis::VertexBuffer m_triangleVB;
	alexis::DynamicConstantBuffer m_triangleCB;
	//alexis::ConstantBuffer m_triangleCB; static
	alexis::RootSignature m_rootSignature;
	std::unique_ptr<alexis::Mesh> m_cubeMesh;

	ComPtr<ID3D12DescriptorHeap> m_imguiSrvHeap;
	ImGuiContext* m_context{ nullptr };

	float m_aspectRatio{ 1.0f };

	SceneConstantBuffer m_constantBufferData;

	float m_timeCount{ 0.f };
	int m_frameCount{ 0 };
	float m_fps{ 0.f };

	DirectX::XMMATRIX m_modelMatrix;
	DirectX::XMMATRIX m_viewMatrix;
	DirectX::XMMATRIX m_projectionMatrix;

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
};