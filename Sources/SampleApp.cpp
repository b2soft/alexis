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

using namespace alexis;
using namespace DirectX;

static const float k_cameraSpeed = 20.0f;
static const float k_cameraTurnSpeed = 0.1f;

enum LightingParameters
{
	GBuffer, //Texture2D 3 textures starting from BaseColor : register( t0 );
	NumLightingParameters
};

enum Hdr2SdrParameters
{
	HDR, //Texture2D 3 textures starting from BaseColor : register( t0 );
	NumHdr2SdrParameters
};

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

	m_gbufferRT.Resize(width, height);
	m_hdrRT.Resize(width, height);
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
	auto commandContext = commandManager->CreateCommandContext();
	auto commandList = commandContext->List.Get();
	//------

	// FS Quad
	m_fsQuad = Mesh::FullScreenQuad(commandContext);

	// Constant Buffer

	//commandContext->CopyBuffer(m_triangleCB, &m_constantBufferData, 1, sizeof(SceneConstantBuffer));
	//commandContext->TransitionResource(m_triangleCB, D3D12_RESOURCE_STATE_GENERIC_READ, true, D3D12_RESOURCE_STATE_COPY_DEST);

	// Textures
	auto resManager = Core::Get().GetResourceManager();
	m_checkerTexture = resManager->GetTexture(L"Resources/Textures/Checker2.dds");

	//commandContext->LoadTextureFromFile(m_checkerTexture, L"Resources/Textures/Checker2.dds");
	//commandContext->TransitionResource(m_checkerTexture, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandContext->LoadTextureFromFile(m_concreteTex, L"Resources/Textures/metal_base.dds");
	commandContext->TransitionResource(m_concreteTex, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandContext->LoadTextureFromFile(m_normalTex, L"Resources/Textures/metal_normal.dds");
	commandContext->TransitionResource(m_normalTex, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandContext->LoadTextureFromFile(m_metalTex, L"Resources/Textures/metal_mero.dds");
	commandContext->TransitionResource(m_metalTex, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

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
		m_gbufferRT.AttachTexture(gb0, RenderTarget::Slot::Slot0);
		gb0.GetResource()->SetName(L"GB 0");
		commandContext->TransitionResource(gb0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		TextureBuffer gb1;
		gb1.Create(colorDesc, &colorClearValue); // Base color
		m_gbufferRT.AttachTexture(gb1, RenderTarget::Slot::Slot1);
		gb1.GetResource()->SetName(L"GB 1");
		commandContext->TransitionResource(gb1, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		TextureBuffer gb2;
		gb2.Create(colorDesc, &colorClearValue); // Base color
		m_gbufferRT.AttachTexture(gb2, RenderTarget::Slot::Slot2);
		gb2.GetResource()->SetName(L"GB 2");
		commandContext->TransitionResource(gb2, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		m_gbufferRT.AttachTexture(depthTexture, RenderTarget::Slot::DepthStencil);
		depthTexture.GetResource()->SetName(L"GB Depth");
		commandContext->TransitionResource(depthTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}

	PBRMaterialParams params =
	{
		.VSPath = L"Resources/Shaders/PBS_object_vs.cso",
		.PSPath = L"Resources/Shaders/PBS_object_ps.cso",
		.RTVFormats = m_gbufferRT.GetFormat(),
		.DSVFormat = depthFormat,
		.t0 = *m_checkerTexture,
		.t1 = m_normalTex,
		.t2 = m_metalTex
	};

	m_pbrMaterial = std::make_unique<PBRMaterial>(params);


	// Create an HDR RT
	{
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

		m_hdrRT.AttachTexture(hdrTexture, RenderTarget::Slot::Slot0);
		commandContext->TransitionResource(hdrTexture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_hdrRT.AttachTexture(depthTexture, RenderTarget::Slot::DepthStencil);
	}

	// Root signature and PSO for Lighting. GBuffer -> HDR
	{
		ComPtr<ID3DBlob> vertexShaderBlob;
		ComPtr<ID3DBlob> pixelShaderBlob;
#if defined(_DEBUG)
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Lighting_vs_d.cso", &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Lighting_ps_d.cso", &pixelShaderBlob));
#else 
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Lighting_vs.cso", &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Lighting_ps.cso", &pixelShaderBlob));
#endif

		// Allow input layout and deny unnecessary access to certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		const auto& gbufferTextures = m_gbufferRT.GetTextures();

		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[LightingParameters::NumLightingParameters];
		rootParameters[LightingParameters::GBuffer].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC linearSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(LightingParameters::NumLightingParameters, rootParameters, 1, &linearSampler, rootSignatureFlags);

		m_lightingSig.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, render->GetHightestSignatureVersion());

		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vs;
			CD3DX12_PIPELINE_STATE_STREAM_PS ps;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
		} pipelineStateStream;

		pipelineStateStream.rootSignature = m_lightingSig.GetRootSignature().Get();
		pipelineStateStream.inputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.rtvFormats = m_hdrRT.GetFormat();

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
		{
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_lightingPSO)));
	}

	// Root signature and PSO for HDR to SDR conversion. HDR -> SDR backbuffer
	{
		// Load the vertex shader
		ComPtr<ID3DBlob> vertexShaderBlob;
		ComPtr<ID3DBlob> pixelShaderBlob;
#if defined(_DEBUG)
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Hdr2Sdr_vs_d.cso", &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Hdr2Sdr_ps_d.cso", &pixelShaderBlob));
#else
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Hdr2Sdr_vs.cso", &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Hdr2Sdr_ps.cso", &pixelShaderBlob));
#endif

		// Allow input layout and deny unnecessary access to certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[Hdr2SdrParameters::NumHdr2SdrParameters];
		rootParameters[Hdr2SdrParameters::HDR].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC linearSampler(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(Hdr2SdrParameters::NumHdr2SdrParameters, rootParameters, 1, &linearSampler, rootSignatureFlags);

		m_hdr2sdrSig.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, render->GetHightestSignatureVersion());

		CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vs;
			CD3DX12_PIPELINE_STATE_STREAM_PS ps;
			CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
		} pipelineStateStream;

		pipelineStateStream.rootSignature = m_hdr2sdrSig.GetRootSignature().Get();
		pipelineStateStream.inputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.Rasterizer = rasterizerDesc;
		pipelineStateStream.rtvFormats = render->GetBackbufferRT().GetFormat();

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
		{
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_hdr2sdrPSO)));
	}

	// Finish loading and wait until all assets are loaded
	commandContext->Flush(true);

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

	auto pbsCommandContext = commandManager->CreateCommandContext();
	auto lightingCommandContext = commandManager->CreateCommandContext();
	auto hdrCommandContext = commandManager->CreateCommandContext();
	auto imguiContext = commandManager->CreateCommandContext();

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

	auto pbsTask = std::async([this, pbsCommandContext, clearColor]()
		{
			{
				auto& gbDepthStencil = m_gbufferRT.GetTexture(RenderTarget::DepthStencil);

				for (int i = RenderTarget::Slot::Slot0; i < RenderTarget::Slot::DepthStencil; ++i)
				{
					auto& texture = m_gbufferRT.GetTexture(static_cast<RenderTarget::Slot>(i));
					if (texture.IsValid())
					{
						pbsCommandContext->TransitionResource(texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
						pbsCommandContext->ClearTexture(texture, clearColor);
					}
				}

				pbsCommandContext->ClearDepthStencil(gbDepthStencil, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0);
			}

			pbsCommandContext->SetRenderTarget(m_gbufferRT);
			pbsCommandContext->SetViewport(m_gbufferRT.GetViewport());
			pbsCommandContext->List->RSSetScissorRects(1, &m_scissorRect);

			XMMATRIX viewProj = XMMatrixMultiply(m_cameraSystem->GetViewMatrix(m_sceneCamera), m_cameraSystem->GetProjMatrix(m_sceneCamera));

			m_pbrMaterial->SetupToRender(pbsCommandContext);

			m_modelSystem->Render(pbsCommandContext, viewProj);
		});

	auto resolveLightingTask = std::async([this, lightingCommandContext, clearColor]
		{
			{
				lightingCommandContext->TransitionResource(m_hdrRT.GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
				lightingCommandContext->ClearTexture(m_hdrRT.GetTexture(RenderTarget::Slot::Slot0), clearColor);
			}

			lightingCommandContext->TransitionResource(m_gbufferRT.GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			lightingCommandContext->TransitionResource(m_gbufferRT.GetTexture(RenderTarget::Slot::Slot1), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
			lightingCommandContext->TransitionResource(m_gbufferRT.GetTexture(RenderTarget::Slot::Slot2), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			lightingCommandContext->SetRenderTarget(m_hdrRT);
			lightingCommandContext->SetViewport(m_hdrRT.GetViewport());
			lightingCommandContext->List->RSSetScissorRects(1, &m_scissorRect);

			lightingCommandContext->List->SetPipelineState(m_lightingPSO.Get());
			lightingCommandContext->SetRootSignature(m_lightingSig);

			lightingCommandContext->SetSRV(LightingParameters::GBuffer, 0, m_gbufferRT.GetTexture(RenderTarget::Slot::Slot0));
			lightingCommandContext->SetSRV(LightingParameters::GBuffer, 1, m_gbufferRT.GetTexture(RenderTarget::Slot::Slot1));
			lightingCommandContext->SetSRV(LightingParameters::GBuffer, 2, m_gbufferRT.GetTexture(RenderTarget::Slot::Slot2));


			m_fsQuad->Draw(lightingCommandContext);
		});


	// Setup render to backbuffer
	const auto& backbuffer = render->GetBackbufferRT();
	const auto& backTexture = backbuffer.GetTexture(RenderTarget::Slot::Slot0);

	auto hdr2sdrTask = std::async([this, hdrCommandContext, &backbuffer, &backTexture]
		{
			hdrCommandContext->TransitionResource(backTexture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

			hdrCommandContext->TransitionResource(m_hdrRT.GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

			hdrCommandContext->List->RSSetScissorRects(1, &m_scissorRect);

			hdrCommandContext->SetRenderTarget(backbuffer);
			hdrCommandContext->SetViewport(backbuffer.GetViewport());

			hdrCommandContext->List->SetPipelineState(m_hdr2sdrPSO.Get());
			hdrCommandContext->SetRootSignature(m_hdr2sdrSig);

			hdrCommandContext->SetSRV(Hdr2SdrParameters::HDR, 0, m_hdrRT.GetTexture(RenderTarget::Slot::Slot0));

			m_fsQuad->Draw(hdrCommandContext);

			hdrCommandContext->TransitionResource(backTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
			//hdrCommandContext->TransitionResource(backTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		});

	auto imguiTask = std::async([this, imguiContext, &backbuffer, &backTexture]
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
		});

	pbsTask.wait();
	resolveLightingTask.wait();
	hdr2sdrTask.wait();
	imguiTask.wait();

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
