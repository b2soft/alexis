#include "Precompiled.h"

#include "SampleApp.h"

#include <CoreHelpers.h>
#include <d3dcompiler.h>
#include <future>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

#include <Scene.h>
#include <Core/Core.h>
#include <Render/Render.h>
#include <Core/CommandManager.h>

#include <Render/Mesh.h>

#include <ECS/ECS.h>
#include <ECS/CameraComponent.h>
#include <ECS/ModelComponent.h>
#include <ECS/TransformComponent.h>
#include <ECS/LightComponent.h>

#include <Core/ResourceManager.h>

using namespace alexis;
using namespace DirectX;

//#define MULTITHREAD_CONTEXTS

static const float k_cameraSpeed = 20.0f;
static const float k_cameraTurnSpeed = 0.1f;

SampleApp::SampleApp() :
	m_viewport(0.0f, 0.0f, static_cast<float>(alexis::g_clientWidth), static_cast<float>(alexis::g_clientHeight)),
	m_scissorRect(0, 0, LONG_MAX, LONG_MAX),
	m_aspectRatio(static_cast<float>(alexis::g_clientWidth) / static_cast<float>(alexis::g_clientHeight))
{
}

bool SampleApp::Initialize()
{
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(Core::GetHwnd(), &p);

	ResetMousePos();

	auto ecsWorld = Core::Get().GetECS();

	ecsWorld->Init();

	ecsWorld->RegisterComponent<ecs::ModelComponent>();
	ecsWorld->RegisterComponent<ecs::TransformComponent>();
	ecsWorld->RegisterComponent<ecs::CameraComponent>();
	ecsWorld->RegisterComponent<ecs::LightComponent>();

	// Model System
	m_modelSystem = ecsWorld->RegisterSystem<ecs::ModelSystem>();

	ecs::ComponentMask modelSystemMask;
	modelSystemMask.set(ecsWorld->GetComponentType<ecs::ModelComponent>());
	modelSystemMask.set(ecsWorld->GetComponentType<ecs::TransformComponent>());
	ecsWorld->SetSystemComponentMask<ecs::ModelSystem>(modelSystemMask);

	// Shadow System
	m_shadowSystem = ecsWorld->RegisterSystem<ecs::ShadowSystem>();

	ecs::ComponentMask shadowSystemMask;
	shadowSystemMask.set(ecsWorld->GetComponentType<ecs::ModelComponent>());
	shadowSystemMask.set(ecsWorld->GetComponentType<ecs::TransformComponent>());
	ecsWorld->SetSystemComponentMask<ecs::ShadowSystem>(shadowSystemMask);

	// Camera System
	m_cameraSystem = ecsWorld->RegisterSystem<ecs::CameraSystem>();

	ecs::ComponentMask cameraSystemMask;
	cameraSystemMask.set(ecsWorld->GetComponentType<ecs::CameraComponent>());
	cameraSystemMask.set(ecsWorld->GetComponentType<ecs::TransformComponent>());
	ecsWorld->SetSystemComponentMask<ecs::CameraSystem>(cameraSystemMask);

	// Lighting System
	m_lightingSystem = ecsWorld->RegisterSystem<ecs::LightingSystem>();

	ecs::ComponentMask lightingSystemMask;
	lightingSystemMask.set(ecsWorld->GetComponentType<ecs::LightComponent>());
	//lightingSystemMask.set(ecsWorld->GetComponentType<ecs::TransformComponent>());
	ecsWorld->SetSystemComponentMask<ecs::LightingSystem>(lightingSystemMask);

	m_hdr2SdrSystem = ecsWorld->RegisterSystem<ecs::Hdr2SdrSystem>();

	m_imguiSystem = ecsWorld->RegisterSystem<ecs::ImguiSystem>();

	return true;
}

void SampleApp::OnUpdate(float dt)
{
	m_modelSystem->Update(dt);

	m_imguiSystem->Update();
	UpdateGUI();

	m_frameCount++;
	m_timeCount += dt;
	m_totalTime += dt;

	if (m_timeCount > 1.f)
	{
		m_fps = m_frameCount / m_timeCount;
		m_timeCount = 0.f;
		m_frameCount = 0;
	}

	if (!m_isCameraFixed)
	{
		m_yaw += m_deltaMouseX * k_cameraTurnSpeed;
		m_pitch += m_deltaMouseY * k_cameraTurnSpeed;

		m_deltaMouseX = 0;
		m_deltaMouseY = 0;

		XMVECTOR posOffset = XMVectorSet(m_rightMovement - m_leftMovement, m_upMovement - m_downMovement, m_fwdMovement - m_aftMovement, 1.0f) * k_cameraSpeed * static_cast<float>(dt);
		auto cameraTransformComponent = Core::Get().GetECS()->GetComponent<ecs::TransformComponent>(m_sceneCamera);
		XMVECTOR newPos = cameraTransformComponent.Position + XMVector3Rotate(posOffset, cameraTransformComponent.Rotation);
		newPos = XMVectorSetW(newPos, 1.0f);

		m_cameraSystem->SetPosition(m_sceneCamera, newPos);

		XMVECTOR cameraRotationQuat = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_pitch), XMConvertToRadians(m_yaw), 0.0f);
		m_cameraSystem->SetRotation(m_sceneCamera, cameraRotationQuat);
	}
}

void SampleApp::OnRender()
{

}

void SampleApp::OnResize(int width, int height)
{
	m_aspectRatio = static_cast<float>(alexis::g_clientWidth) / static_cast<float>(alexis::g_clientHeight);

	m_cameraSystem->SetProjectionParams(m_sceneCamera, 45.0f, m_aspectRatio, 0.1f, 100.0f);

	Render::GetInstance()->GetRTManager()->Resize(width, height);
}

void SampleApp::OnKeyPressed(alexis::KeyEventArgs& e)
{
	IGame::OnKeyPressed(e);

	switch (e.Key)
	{

	case KeyCode::Escape:
	{
		alexis::Core::Get().Quit();
	}
	break;

	case KeyCode::Enter:
	{
		if (e.Alt)
		{
			alexis::Render::GetInstance()->ToggleFullscreen();
		}
	}
	break;

	case KeyCode::F11:
	{
		alexis::Render::GetInstance()->ToggleFullscreen();
	}
	break;

	case KeyCode::V:
	{
		alexis::Render::GetInstance()->ToggleVSync();
	}
	break;

	case KeyCode::W:
	{
		m_fwdMovement = 1.0f;
	}
	break;

	case KeyCode::S:
	{
		m_aftMovement = 1.0f;
	}
	break;

	case KeyCode::A:
	{
		m_leftMovement = 1.0f;
	}
	break;

	case KeyCode::D:
	{
		m_rightMovement = 1.0f;
	}
	break;

	case KeyCode::R:
	{
		m_yaw = 0.0f;
		m_pitch = 0.0f;
	}
	break;

	case KeyCode::F:
	{
		ToggleFixedCamera();
	}
	break;

	}
}

void SampleApp::OnKeyReleased(KeyEventArgs& e)
{
	IGame::OnKeyReleased(e);

	switch (e.Key)
	{

	case KeyCode::W:
	{
		m_fwdMovement = 0.0f;
	}
	break;

	case KeyCode::S:
	{
		m_aftMovement = 0.0f;
	}
	break;

	case KeyCode::A:
	{
		m_leftMovement = 0.0f;
	}
	break;

	case KeyCode::D:
	{
		m_rightMovement = 0.0f;
	}
	break;

	case KeyCode::R:
	{
		m_upMovement = 0.0f;
	}
	break;

	case KeyCode::F:
	{
		m_downMovement = 0.0f;
	}
	break;

	}
}

void SampleApp::OnMouseMoved(alexis::MouseMotionEventArgs& e)
{
	if (!m_isCameraFixed)
	{
		int windowCenterX = alexis::g_clientWidth / 2;
		int windowCenterY = alexis::g_clientHeight / 2;

		if (e.X == windowCenterX &&
			e.Y == windowCenterY)
		{
			return;
		}

		m_deltaMouseX = e.X - windowCenterX;
		m_deltaMouseY = e.Y - windowCenterY;

		ResetMousePos();
	}
}

void SampleApp::Destroy()
{
}

void SampleApp::LoadPipeline()
{

}

void SampleApp::LoadAssets()
{
	//----- Loading
	auto render = alexis::Render::GetInstance();
	//------

	// Depth
	//DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
	DXGI_FORMAT depthFormat = DXGI_FORMAT_R24G8_TYPELESS;
	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthFormat, alexis::g_clientWidth, alexis::g_clientHeight);
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.0f, 0 };

	TextureBuffer depthTexture;
	depthTexture.Create(depthDesc, &depthClearValue);

	// Create a G-Buffer
	{
		auto gbuffer = std::make_unique<RenderTarget>();

		//DXGI_FORMAT gbufferColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT gbufferColorFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
		auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(gbufferColorFormat, alexis::g_clientWidth, alexis::g_clientHeight, 1, 1);
		colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = colorDesc.Format;
		colorClearValue.Color[0] = 0.0f;
		colorClearValue.Color[1] = 0.0f;
		colorClearValue.Color[2] = 0.5f;
		colorClearValue.Color[3] = 1.0f;

		TextureBuffer gb0;
		gb0.Create(colorDesc, &colorClearValue); // Base color
		gbuffer->AttachTexture(gb0, RenderTarget::Slot::Slot0);
		gb0.GetResource()->SetName(L"GB 0");

		TextureBuffer gb1;
		gb1.Create(colorDesc, &colorClearValue); // Normals
		gbuffer->AttachTexture(gb1, RenderTarget::Slot::Slot1);
		gb1.GetResource()->SetName(L"GB 1");

		TextureBuffer gb2;
		gb2.Create(colorDesc, &colorClearValue); // Base color
		gbuffer->AttachTexture(gb2, RenderTarget::Slot::Slot2);
		gb2.GetResource()->SetName(L"GB 2");

		gbuffer->AttachTexture(depthTexture, RenderTarget::Slot::DepthStencil);
		depthTexture.GetResource()->SetName(L"GB Depth");

		render->GetRTManager()->EmplaceTarget(L"GB", std::move(gbuffer));
	}

	// Create an HDR RT
	{
		auto hdrTarget = std::make_unique<RenderTarget>();
		DXGI_FORMAT hdrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

		auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(hdrFormat, alexis::g_clientWidth, alexis::g_clientHeight, 1, 1);
		colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = colorDesc.Format;
		colorClearValue.Color[0] = 0.0f;
		colorClearValue.Color[1] = 0.0f;
		colorClearValue.Color[2] = 0.0f;
		colorClearValue.Color[3] = 1.0f;

		TextureBuffer hdrTexture;
		hdrTexture.Create(colorDesc, &colorClearValue);
		hdrTexture.GetResource()->SetName(L"HDR Texture");

		hdrTarget->AttachTexture(hdrTexture, RenderTarget::Slot::Slot0);
		//hdrTarget->AttachTexture(depthTexture, RenderTarget::Slot::DepthStencil);

		render->GetRTManager()->EmplaceTarget(L"HDR", std::move(hdrTarget));
	}

	// Create shadow map target
	{
		DXGI_FORMAT shadowFormat = DXGI_FORMAT_R24G8_TYPELESS;
		auto shadowDesc = CD3DX12_RESOURCE_DESC::Tex2D(shadowFormat, 1024, 1024);
		shadowDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		D3D12_CLEAR_VALUE shadowDepthClearValue;
		shadowDepthClearValue.Format = shadowDesc.Format;
		shadowDepthClearValue.DepthStencil = { 1.0f, 0 };

		TextureBuffer shadowDepthTexture;
		shadowDepthTexture.Create(shadowDesc, &shadowDepthClearValue);

		auto shadowRT = std::make_unique<RenderTarget>();
		shadowRT->AttachTexture(shadowDepthTexture, RenderTarget::DepthStencil);
		render->GetRTManager()->EmplaceTarget(L"Shadow Map", std::move(shadowRT));
	}

	m_lightingSystem->Init();
	m_shadowSystem->Init();
	m_hdr2SdrSystem->Init();
	m_imguiSystem->Init();


	// Create Synchronization Objects
	{
		render->GetCommandManager()->WaitForGpu();
	}
}



void SampleApp::UpdateGUI()
{
	static bool showDemoWindow = false;

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit", "Esc"))
			{
				alexis::Core::Get().Quit(0);
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow);

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Options"))
		{
			bool vSync = Render::GetInstance()->IsVSync();
			if (ImGui::MenuItem("V-Sync", "V", &vSync))
			{
				Render::GetInstance()->SetVSync(vSync);
			}

			bool fullscreen = Render::GetInstance()->IsFullscreen();
			if (ImGui::MenuItem("Full screen", "Alt+Enter", &fullscreen))
			{
				Render::GetInstance()->SetFullscreen(fullscreen);
			}

			ImGui::EndMenu();
		}

		char buffer[256];


		{
			sprintf_s(buffer, _countof(buffer), "FPS: %.2f (%.2f ms) Frame: %I64i  ", m_fps, 1.0 / m_fps * 1000.0, alexis::Core::GetFrameCount());
			auto fpsTextSize = ImGui::CalcTextSize(buffer);
			ImGui::SameLine(ImGui::GetWindowWidth() - fpsTextSize.x);
			ImGui::Text(buffer);
		}

		{
			ImGui::Begin("BottomBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs |
				ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);

			auto cameraTransformComponent = Core::Get().GetECS()->GetComponent<ecs::TransformComponent>(m_sceneCamera);
			sprintf_s(buffer, _countof(buffer), "Camera Pos{ X: %.2f Y: %.2f Z: %.2f }", XMVectorGetX(cameraTransformComponent.Position), XMVectorGetY(cameraTransformComponent.Position), XMVectorGetZ(cameraTransformComponent.Position));
			ImGui::TextColored(ImVec4(1.0, 1.0, 1.0, 1.0), buffer);

			auto posTextSize = ImGui::CalcTextSize(buffer);

			ImGui::SetWindowSize(ImVec2(static_cast<float>(g_clientWidth), posTextSize.y));
			ImGui::SetWindowPos(ImVec2(0, g_clientHeight - posTextSize.y - 10.0f));

			ImGui::End();
		}

		ImGui::EndMainMenuBar();
	}

	//if ( ImGui::)

	if (showDemoWindow)
	{
		ImGui::ShowDemoWindow(&showDemoWindow);
	}
}

void SampleApp::ToggleFixedCamera()
{
	m_isCameraFixed = !m_isCameraFixed;

	ResetMousePos();

	ShowCursor(m_isCameraFixed);
}

void SampleApp::ResetMousePos()
{
	POINT p;
	p.x = alexis::g_clientWidth / 2;
	p.y = alexis::g_clientHeight / 2;

	ClientToScreen(Core::GetHwnd(), &p);
	SetCursorPos(p.x, p.y);
}

bool SampleApp::LoadContent()
{
	LoadPipeline();
	LoadAssets();

	auto scene = Core::Get().GetScene();
	//scene->LoadFromJson(L"Resources/main.scene");
	scene->LoadFromJson(L"Resources/shaderball.scene");
	//scene->LoadFromJson(L"Resources/main_sphere.scene");

	m_sceneCamera = m_cameraSystem->GetActiveCamera();

	return true;
}

void SampleApp::UnloadContent()
{

}
