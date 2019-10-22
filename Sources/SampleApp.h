#pragma once

#include <d3dx12.h>
#include <DirectXMath.h>

#include <Core/Core.h>
#include <Core/Events.h>

#include <Render/RootSignature.h>
#include <Render/Buffers/GpuBuffer.h>
#include <Render/Mesh.h>
#include <Render/RenderTarget.h>
#include <Render/Camera.h>

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
	virtual void OnResize(int width, int height) override;

	virtual void OnKeyPressed(alexis::KeyEventArgs& e) override;
	virtual void OnKeyReleased(alexis::KeyEventArgs& e) override;

	virtual void OnMouseMoved(alexis::MouseMotionEventArgs& e) override;
	virtual void OnMouseButtonPressed(alexis::MouseButtonEventArgs& e) override;

	virtual void Destroy() override;

private:
	static const UINT k_frameCount = 2;

	// Pipeline objects
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	ComPtr<ID3D12PipelineState> m_pipelineState;

	ComPtr<ID3D12PipelineState> m_pbsObjectPSO;
	ComPtr<ID3D12PipelineState> m_lightingPSO;
	ComPtr<ID3D12PipelineState> m_hdr2sdrPSO;

	alexis::RenderTarget m_gbufferRT;
	alexis::RenderTarget m_hdrRT;

	// Signatures
	alexis::RootSignature m_pbsObjectSig;
	alexis::RootSignature m_lightingSig;
	alexis::RootSignature m_hdr2sdrSig;

	// Textures
	alexis::TextureBuffer m_checkerTexture;
	alexis::TextureBuffer m_normalTex;
	alexis::TextureBuffer m_metalTex;
	alexis::TextureBuffer m_concreteTex;

	alexis::VertexBuffer m_triangleVB;
	alexis::DynamicConstantBuffer m_triangleCB;
	//alexis::ConstantBuffer m_triangleCB; static
	alexis::RootSignature m_rootSignature;
	std::unique_ptr<alexis::Mesh> m_cubeMesh;
	std::unique_ptr<alexis::Mesh> m_fsQuad;

	ComPtr<ID3D12DescriptorHeap> m_imguiSrvHeap;
	ImGuiContext* m_context{ nullptr };

	float m_aspectRatio{ 1.0f };

	SceneConstantBuffer m_constantBufferData;

	float m_timeCount{ 0.f };
	int m_frameCount{ 0 };
	float m_fps{ 0.f };
	float m_totalTime{ 0.f };

	DirectX::XMMATRIX m_modelMatrix;

	alexis::Camera m_sceneCamera;


	// Input system 

	float m_fwdMovement{ 0.0f };
	float m_aftMovement{ 0.0f };
	float m_leftMovement{ 0.0f };
	float m_rightMovement{ 0.0f };
	float m_upMovement{ 0.0f };
	float m_downMovement{ 0.0f };

	float m_pitch{ 0.0f };
	float m_yaw{ 0.0f };

	int m_prevMouseX{ 0 };
	int m_prevMouseY{ 0 };

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();

	void UpdateGUI();
};