#include "EditorApp.h"

#include <filesystem>
#include <DirectXMath.h>

#include <ECS/ECS.h>
#include <ECS/Systems/CameraSystem.h>
#include <ECS/Components/DoNotSerializeComponent.h>
#include <Core/KeyCodes.h>
#include <Render/Render.h>

#include "ECS/EditorSystem.h"
#include "ECS/EditorSystemSerializer.h"

bool EditorApp::Initialize()
{
	auto& ecsWorld = alexis::Core::Get().GetECSWorld();
	m_editorSystem = ecsWorld.RegisterSystem<editor::EditorSystem>();

	alexis::ecs::ComponentMask editorSystemMask;
	ecsWorld.SetSystemComponentMask<editor::EditorSystem>(editorSystemMask);

	m_editorSystem->Init();

	return true;
}

void EditorApp::OnResize(int width, int height)
{
	using namespace alexis;
	auto& ecsWorld = Core::Get().GetECSWorld();
	const auto& editorSystem = ecsWorld.GetSystem<editor::EditorSystem>();

	editorSystem->OnResize(width, height);
}

void EditorApp::OnUpdate(float dt)
{
	// Frame counter
	m_updateCount++;
	m_updateTimeCount += dt;

	if (m_updateTimeCount > 1.f)
	{
		m_ups = static_cast<float>(m_updateCount) / m_updateTimeCount;
		m_updateTimeCount = 0.f;
		m_updateCount = 0;
	}

	if (ImGui::BeginMainMenuBar())
	{
		// Editor
		if (ImGui::BeginMenu("Editor"))
		{
			if (ImGui::BeginMenu("Open"))
			{
				std::string path = "Resources\\Scenes"; // TODO: Remove hardcode later
				for (const auto& filePath : std::filesystem::directory_iterator(path))
				{
					std::wstring pathWStr = filePath.path();
					std::string pathStr{ pathWStr.cbegin(), pathWStr.cend() };
					if (ImGui::MenuItem(pathStr.c_str()))
					{
						LoadScene(pathWStr);
					}
				}

				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Save", "Ctrl+S"))
			{
				SaveScene();
			}
			ImGui::EndMenu();
		}

		// General options
		if (ImGui::BeginMenu("Options"))
		{
			bool vSync = alexis::Render::GetInstance()->IsVSync();
			if (ImGui::MenuItem("V-Sync", "V", &vSync))
			{
				alexis::Render::GetInstance()->SetVSync(vSync);
			}

			bool fullscreen = alexis::Render::GetInstance()->IsFullscreen();
			if (ImGui::MenuItem("Full screen", "Alt+Enter", &fullscreen))
			{
				alexis::Render::GetInstance()->SetFullscreen(fullscreen);
			}

			ImGui::EndMenu();
		}

		// FPS topbar
		char buffer[256];
		{
			sprintf_s(buffer, _countof(buffer), "FPS: %.2f (%.2f ms) | UPS: %.2f (%.2f ms)", m_fps, 1.0 / m_fps * 1000.f, m_ups, 1.0 / m_ups * 1000.f);
			auto fpsTextSize = ImGui::CalcTextSize(buffer);
			ImGui::SameLine(ImGui::GetWindowWidth() - fpsTextSize.x - 20.f);
			ImGui::Text(buffer);
		}

		ImGui::EndMainMenuBar();
	}

	// ECS related
	m_editorSystem->Update(dt);
}

void EditorApp::OnRender(float frameTime)
{
	// Frame counter
	m_frameCount++;
	m_frameTimeCount += frameTime;

	if (m_frameTimeCount > 1.f)
	{
		m_fps = static_cast<float>(m_frameCount) / m_frameTimeCount;
		m_frameTimeCount = 0.f;
		m_frameCount = 0;
	}
}

void EditorApp::OnKeyPressed(alexis::KeyEventArgs& e)
{
	IGame::OnKeyPressed(e);

	using namespace alexis;

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
		m_movement[0] = 1.0f;
		m_editorSystem->SetMovement(m_movement);
	}
	break;

	case KeyCode::S:
	{
		m_movement[1] = 1.0f;
		m_editorSystem->SetMovement(m_movement);
	}
	break;

	case KeyCode::A:
	{
		m_movement[2] = 1.0f;
		m_editorSystem->SetMovement(m_movement);
	}
	break;

	case KeyCode::D:
	{
		m_movement[3] = 1.0f;
		m_editorSystem->SetMovement(m_movement);
	}
	break;

	case KeyCode::F:
	{
		ToggleFixedCamera();
	}
	break;

	}
}

void EditorApp::OnKeyReleased(alexis::KeyEventArgs& e)
{
	IGame::OnKeyReleased(e);

	using namespace alexis;

	switch (e.Key)
	{

	case KeyCode::W:
	{
		m_movement[0] = 0.0f;
		m_editorSystem->SetMovement(m_movement);
	}
	break;

	case KeyCode::S:
	{
		m_movement[1] = 0.0f;
		m_editorSystem->SetMovement(m_movement);
	}
	break;

	case KeyCode::A:
	{
		m_movement[2] = 0.0f;
		m_editorSystem->SetMovement(m_movement);
	}
	break;

	case KeyCode::D:
	{
		m_movement[3] = 0.0f;
		m_editorSystem->SetMovement(m_movement);
	}
	break;

	}
}

void EditorApp::OnMouseMoved(alexis::MouseMotionEventArgs& e)
{
	if (!m_editorSystem->IsCameraFixed())
	{
		int windowCenterX = alexis::g_clientWidth / 2;
		int windowCenterY = alexis::g_clientHeight / 2;

		if (e.X == windowCenterX &&
			e.Y == windowCenterY)
		{
			return;
		}

		m_editorSystem->OnMouseMoved(e.X - windowCenterX, e.Y - windowCenterY);

		ResetMousePos();
	}
}

void EditorApp::ResetMousePos()
{
	POINT p;
	p.x = alexis::g_clientWidth / 2;
	p.y = alexis::g_clientHeight / 2;

	ClientToScreen(alexis::Core::GetHwnd(), &p);
	SetCursorPos(p.x, p.y);
}

void EditorApp::ToggleFixedCamera()
{
	m_editorSystem->ToggleFixedCamera();

	ResetMousePos();

	ShowCursor(m_editorSystem->IsCameraFixed());
}

void EditorApp::LoadScene(std::wstring_view path)
{
	UnloadScene();
	editor::EditorSystemSerializer::Load(std::wstring(path));
	m_loadedScene = { std::wstring(path) };

	const auto& cameraSystem = alexis::Core::Get().GetECSWorld().GetSystem<alexis::ecs::CameraSystem>();
	cameraSystem->SetActiveCamera(m_editorSystem->GetActiveCamera());
}

void EditorApp::UnloadScene()
{
	if (!m_loadedScene.has_value())
	{
		return;
	}

	auto& ecsWorld = alexis::Core::Get().GetECSWorld();
	const auto& entities = m_editorSystem->GetEntityList(); // return cloned entity ids
	for (auto entity : entities)
	{
		if (ecsWorld.HasComponent<alexis::ecs::DoNotSerializeComponent>(entity))
		{
			continue;
		}

		ecsWorld.DestroyEntity(entity);
	}
}

void EditorApp::SaveScene()
{
	editor::EditorSystemSerializer::Save(m_loadedScene->Path);
}