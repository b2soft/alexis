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
#include <Core/Render.h>
#include <Core/CommandManager.h>

#include <Render/Mesh.h>

#include <ECS/ECS.h>
#include <ECS/CameraComponent.h>
#include <ECS/ModelComponent.h>
#include <ECS/TransformComponent.h>

#include <Core/ResourceManager.h>
#include <Render/Materials/Hdr2SdrMaterial.h>

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

	// Model System
	m_modelSystem = ecsWorld->RegisterSystem<ecs::ModelSystem>();

	ecs::ComponentMask modelSystemMask;
	modelSystemMask.set(ecsWorld->GetComponentType<ecs::ModelComponent>());
	modelSystemMask.set(ecsWorld->GetComponentType<ecs::TransformComponent>());
	ecsWorld->SetSystemComponentMask<ecs::ModelSystem>(modelSystemMask);

	// Camera System
	m_cameraSystem = ecsWorld->RegisterSystem<ecs::CameraSystem>();

	ecs::ComponentMask cameraSystemMask;
	cameraSystemMask.set(ecsWorld->GetComponentType<ecs::CameraComponent>());
	cameraSystemMask.set(ecsWorld->GetComponentType<ecs::TransformComponent>());
	ecsWorld->SetSystemComponentMask<ecs::CameraSystem>(cameraSystemMask);

	// Lighting System
	m_lightingSystem = ecsWorld->RegisterSystem<ecs::LightingSystem>();

	ecs::ComponentMask lightingSystemMask;
	//cameraSystemMask.set(ecsWorld->GetComponentType<ecs::CameraComponent>());
	lightingSystemMask.set(ecsWorld->GetComponentType<ecs::TransformComponent>());
	ecsWorld->SetSystemComponentMask<ecs::LightingSystem>(lightingSystemMask);

	return true;
}

void SampleApp::OnUpdate(float dt)
{
	m_modelSystem->Update(dt);

	ImGui::SetCurrentContext(m_context);
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

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
	// Records commands
	PopulateCommandList();
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
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext(m_context);
}

void SampleApp::LoadPipeline()
{
	auto device = alexis::Render::GetInstance()->GetDevice();

	// Create Descriptor Heaps
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_imguiSrvHeap)));
	}
}

void SampleApp::LoadAssets()
{
	//----- Loading
	auto render = alexis::Render::GetInstance();
	auto device = render->GetDevice();
	auto commandManager = render->GetCommandManager();
	//------

	// Depth
	DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
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

		DXGI_FORMAT gbufferColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(gbufferColorFormat, alexis::g_clientWidth, alexis::g_clientHeight, 1, 1);
		colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = colorDesc.Format;
		colorClearValue.Color[0] = 0.0f;
		colorClearValue.Color[1] = 0.2f;
		colorClearValue.Color[2] = 0.4f;
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
		colorClearValue.Color[1] = 0.2f;
		colorClearValue.Color[2] = 0.4f;
		colorClearValue.Color[3] = 1.0f;

		TextureBuffer hdrTexture;
		hdrTexture.Create(colorDesc, &colorClearValue);
		hdrTexture.GetResource()->SetName(L"HDR Texture");

		hdrTarget->AttachTexture(hdrTexture, RenderTarget::Slot::Slot0);
		hdrTarget->AttachTexture(depthTexture, RenderTarget::Slot::DepthStencil);

		render->GetRTManager()->EmplaceTarget(L"HDR", std::move(hdrTarget));
	}

	m_hdr2sdrMaterial = std::make_unique<Hdr2SdrMaterial>();

	// FS Quad
	
	m_fsQuad = Core::Get().GetResourceManager()->GetMesh(L"$FS_QUAD");

	m_lightingSystem->Init();

	IMGUI_CHECKVERSION();
	m_context = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsLight();

	ImGui_ImplWin32_Init(Core::GetHwnd());
	ImGui_ImplDX12_Init(Render::GetInstance()->GetDevice(), 2,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_imguiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_imguiSrvHeap->GetGPUDescriptorHandleForHeapStart());


	// Create Synchronization Objects
	{
		render->GetCommandManager()->WaitForGpu();
	}
}

void SampleApp::PopulateCommandList()
{
	auto render = alexis::Render::GetInstance();
	auto commandManager = render->GetCommandManager();

	auto clearTargetCommandContext = commandManager->CreateCommandContext();
	auto pbsCommandContext = commandManager->CreateCommandContext();
	auto lightingCommandContext = commandManager->CreateCommandContext();
	auto hdrCommandContext = commandManager->CreateCommandContext();
	auto imguiContext = commandManager->CreateCommandContext();

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

	auto gbuffer = render->GetRTManager()->GetRenderTarget(L"GB");
	auto hdrRT = render->GetRTManager()->GetRenderTarget(L"HDR");

#if defined(MULTITHREAD_CONTEXTS)
	auto clearTargetsTask = std::async([this, clearTargetCommandContext, gbuffer, hdrRT, clearColor]()
#endif
		{
			// Clear G-Buffer
			auto& gbDepthStencil = gbuffer->GetTexture(RenderTarget::DepthStencil);

			for (int i = RenderTarget::Slot::Slot0; i < RenderTarget::Slot::DepthStencil; ++i)
			{
				auto& texture = gbuffer->GetTexture(static_cast<RenderTarget::Slot>(i));
				if (texture.IsValid())
				{
					clearTargetCommandContext->TransitionResource(texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
					clearTargetCommandContext->ClearTexture(texture, clearColor);
				}
			}

			clearTargetCommandContext->TransitionResource(gbDepthStencil, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
			clearTargetCommandContext->ClearDepthStencil(gbDepthStencil, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0);

			// Clear HDR
			auto& texture = hdrRT->GetTexture(static_cast<RenderTarget::Slot>(RenderTarget::Slot::Slot0));
			clearTargetCommandContext->TransitionResource(texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
			clearTargetCommandContext->ClearTexture(texture, clearColor);
		}
#if defined(MULTITHREAD_CONTEXTS)
	);
#endif

#if defined(MULTITHREAD_CONTEXTS)
	auto pbsTask = std::async([this, pbsCommandContext, clearColor, gbuffer]()
#endif
		{
			pbsCommandContext->SetRenderTarget(*gbuffer);
			pbsCommandContext->SetViewport(gbuffer->GetViewport());
			pbsCommandContext->List->RSSetScissorRects(1, &m_scissorRect);

			XMMATRIX viewProj = XMMatrixMultiply(m_cameraSystem->GetViewMatrix(m_sceneCamera), m_cameraSystem->GetProjMatrix(m_sceneCamera));

			m_modelSystem->Render(pbsCommandContext, viewProj);
		}
#if defined(MULTITHREAD_CONTEXTS)
	);
#endif

#if defined(MULTITHREAD_CONTEXTS)
	auto resolveLightingTask = std::async([this, lightingCommandContext, clearColor]
#endif
		{
			lightingCommandContext->List->RSSetScissorRects(1, &m_scissorRect);
			m_lightingSystem->Render(lightingCommandContext);
		}
#if defined(MULTITHREAD_CONTEXTS)
	);
#endif


	const auto& backbuffer = render->GetBackbufferRT();
	const auto& backTexture = backbuffer.GetTexture(RenderTarget::Slot::Slot0);

#if defined(MULTITHREAD_CONTEXTS)
	auto hdr2sdrTask = std::async([this, hdrCommandContext, &backbuffer, &backTexture]
#endif
		{
			hdrCommandContext->List->RSSetScissorRects(1, &m_scissorRect);
			// TODO create HDR2SDR system
			hdrCommandContext->TransitionResource(backTexture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
			
			hdrCommandContext->SetRenderTarget(backbuffer);
			hdrCommandContext->SetViewport(backbuffer.GetViewport());

			m_hdr2sdrMaterial->SetupToRender(hdrCommandContext);

			m_fsQuad->Draw(hdrCommandContext);

			hdrCommandContext->TransitionResource(backTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
#if defined(MULTITHREAD_CONTEXTS)
	);
#endif

#if defined(MULTITHREAD_CONTEXTS)
	auto imguiTask = std::async([this, imguiContext, &backbuffer, &backTexture]
#endif
		{
			imguiContext->TransitionResource(backTexture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

			imguiContext->SetRenderTarget(backbuffer);
			imguiContext->SetViewport(backbuffer.GetViewport());
			imguiContext->List->RSSetScissorRects(1, &m_scissorRect);

			ID3D12DescriptorHeap* imguiHeap[] = { m_imguiSrvHeap.Get() };
			imguiContext->List->SetDescriptorHeaps(_countof(imguiHeap), imguiHeap);
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), imguiContext->List.Get());

			imguiContext->TransitionResource(backTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		}
#if defined(MULTITHREAD_CONTEXTS)
	);
#endif

#if defined(MULTITHREAD_CONTEXTS)
	clearTargetsTask.wait();
	pbsTask.wait();
	resolveLightingTask.wait();
	hdr2sdrTask.wait();
	imguiTask.wait();
#endif

	clearTargetCommandContext->Finish();
	pbsCommandContext->Finish();
	lightingCommandContext->Finish();
	hdrCommandContext->Finish();
	imguiContext->Finish();
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

			ImGui::SetWindowSize(ImVec2(g_clientWidth, posTextSize.y));
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
	scene->LoadFromJson(L"Resources/main.scene");

	m_sceneCamera = m_cameraSystem->GetActiveCamera();

	return true;
}

void SampleApp::UnloadContent()
{

}
