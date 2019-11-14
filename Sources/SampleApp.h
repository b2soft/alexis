#pragma once

#include <d3dx12.h>
#include <DirectXMath.h>

#include <Core/Core.h>
#include <Core/Events.h>

#include <Render/RootSignature.h>
#include <Render/Buffers/GpuBuffer.h>
#include <Render/Mesh.h>
#include <Render/RenderTarget.h>

#include <ECS/ModelSystem.h>
#include <ECS/CameraSystem.h>

#include <Render/Materials/MaterialBase.h>

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

	virtual void Destroy() override;

private:
	static const UINT k_frameCount = 3;

	// Pipeline objects
	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	std::unique_ptr<alexis::MaterialBase> m_lightingMaterial;
	std::unique_ptr<alexis::MaterialBase> m_hdr2sdrMaterial;

	std::unique_ptr<alexis::Mesh> m_fsQuad;

	alexis::ecs::Entity m_sceneCamera;

	ComPtr<ID3D12DescriptorHeap> m_imguiSrvHeap;
	ImGuiContext* m_context{ nullptr };

	float m_aspectRatio{ 1.0f };

	float m_timeCount{ 0.f };
	int m_frameCount{ 0 };
	float m_fps{ 0.f };
	float m_totalTime{ 0.f };

	// Input system 
	float m_fwdMovement{ 0.0f };
	float m_aftMovement{ 0.0f };
	float m_leftMovement{ 0.0f };
	float m_rightMovement{ 0.0f };
	float m_upMovement{ 0.0f };
	float m_downMovement{ 0.0f };

	float m_pitch{ 0.0f };
	float m_yaw{ 0.0f };

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();

	void UpdateGUI();

	void ToggleFixedCamera();
	void ResetMousePos();
	int m_deltaMouseX{ 0 };
	int m_deltaMouseY{ 0 };
	bool m_isCameraFixed{ true };

	std::shared_ptr<alexis::ecs::ModelSystem> m_modelSystem;
	std::shared_ptr<alexis::ecs::CameraSystem> m_cameraSystem;
};