#include "Precompiled.h"

#include "SampleApp.h"

#include <Core/Core.h>
#include <Render/Render.h>

#include <Render/Mesh.h>

#include <ECS/ECS.h>
#include <ECS/Systems/CameraSystem.h>
#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/TransformComponent.h>

using namespace alexis;
using namespace DirectX;

//#define MULTITHREAD_CONTEXTS

static const float k_cameraSpeed = 10.0f;
static const float k_cameraTurnSpeed = 0.1f;

SampleApp::SampleApp() :
	m_aspectRatio(static_cast<float>(alexis::g_clientWidth) / static_cast<float>(alexis::g_clientHeight))
{
}

bool SampleApp::Initialize()
{
	POINT p;
	GetCursorPos(&p);
	ScreenToClient(Core::GetHwnd(), &p);

	ResetMousePos();

	return true;
}

void SampleApp::OnUpdate(float dt)
{
	UpdateGUI();

	m_frameCount++;
	m_timeCount += dt;
	m_totalTime += dt;

	if (m_timeCount > 1.f)
	{
		m_fps = static_cast<float>(m_frameCount) / m_timeCount;
		m_timeCount = 0.f;
		m_frameCount = 0;
	}

	if (!m_isCameraFixed)
	{
		m_yaw += static_cast<float>(m_deltaMouseX) * k_cameraTurnSpeed;
		m_pitch += static_cast<float>(m_deltaMouseY) * k_cameraTurnSpeed;

		m_deltaMouseX = 0;
		m_deltaMouseY = 0;

		auto& ecsWorld = Core::Get().GetECSWorld();

		XMVECTOR posOffset = XMVectorSet(m_rightMovement - m_leftMovement, m_upMovement - m_downMovement, m_fwdMovement - m_aftMovement, 1.0f) * k_cameraSpeed * static_cast<float>(dt);
		auto cameraTransformComponent = ecsWorld.GetComponent<ecs::TransformComponent>(m_sceneCamera);
		XMVECTOR newPos = cameraTransformComponent.Position + XMVector3Rotate(posOffset, cameraTransformComponent.Rotation);
		newPos = XMVectorSetW(newPos, 1.0f);

		auto cameraSystem = ecsWorld.GetSystem<alexis::ecs::CameraSystem>();
		cameraSystem->SetPosition(m_sceneCamera, newPos);

		XMVECTOR cameraRotationQuat = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_pitch), XMConvertToRadians(m_yaw), 0.0f);
		cameraSystem->SetRotation(m_sceneCamera, cameraRotationQuat);
	}
}

void SampleApp::OnRender()
{
}

void SampleApp::OnResize(int width, int height)
{
	m_aspectRatio = static_cast<float>(alexis::g_clientWidth) / static_cast<float>(alexis::g_clientHeight);

	auto& ecsWorld = Core::Get().GetECSWorld();
	auto cameraSystem = ecsWorld.GetSystem<alexis::ecs::CameraSystem>();

	cameraSystem->SetProjectionParams(m_sceneCamera, 45.0f, m_aspectRatio, 0.1f, 100.0f);
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
			//ImGui::Begin("BottomBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs |
			//	ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
			//
			//auto cameraTransformComponent = Core::Get().GetECSWorld().GetComponent<ecs::TransformComponent>(m_sceneCamera);
			//sprintf_s(buffer, _countof(buffer), "Camera Pos{ X: %.2f Y: %.2f Z: %.2f }", XMVectorGetX(cameraTransformComponent.Position), XMVectorGetY(cameraTransformComponent.Position), XMVectorGetZ(cameraTransformComponent.Position));
			//ImGui::TextColored(ImVec4(1.0, 1.0, 1.0, 1.0), buffer);
			//
			//auto posTextSize = ImGui::CalcTextSize(buffer);
			//
			//ImGui::SetWindowSize(ImVec2(static_cast<float>(g_clientWidth), posTextSize.y));
			//ImGui::SetWindowPos(ImVec2(0, g_clientHeight - posTextSize.y - 10.0f));
			//
			//ImGui::End();
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
