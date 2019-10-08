#include "Precompiled.h"

#include <d3dcompiler.h>
#include <wrl.h>
#include <chrono>

#include "Application.h"
#include "CommandQueue.h"
#include "Helpers.h"
#include "Window.h"

#include "Render/CommandList.h"
#include "Render/Mesh.h"

#include "b2Game.h"

#include <imgui.h>

using namespace Microsoft::WRL;
using namespace DirectX;

struct Mat
{
	XMMATRIX ModelMatrix;
	XMMATRIX ModelViewMatrix;
	XMMATRIX ModelViewProjectionMatrix;
};

enum PBSObjectParameters
{
	MatricesCB, //ConstantBuffer<Mat> MatCB: register(b0);
	Textures, //Texture2D 3 textures starting from BaseColor : register( t0 );
	NumPBSObjectParameters
};

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

static double g_FPS = 0.0;

b2Game::b2Game(const std::wstring& name, int width, int height, bool vSync /*= false*/)
	: Game(name, width, height, vSync)
	, m_scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
	, m_viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
	, m_fov(45.0f)
	, m_contentLoaded(false)
	, m_width(width)
	, m_height(height)
{
}

void b2Game::OnUpdate(UpdateEventArgs& e)
{
	static uint64_t frameCount = 0;
	static double totalTime = 0.0;

	Game::OnUpdate(e);

	totalTime += e.ElapsedTime;
	frameCount++;

	if (totalTime > 1.0)
	{
		g_FPS = frameCount / totalTime;

		frameCount = 0;
		totalTime = 0.0;
	}

	// Update the model matrix
	float angle = static_cast<float>(e.TotalTime * 90.0);
	const XMVECTOR rotationAxis = XMVectorSet(1.0f, 1.0f, 0.0f, 0.0f);
	m_modelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

	// Update view matrix
	const XMVECTOR eyePosition = XMVectorSet(0.0f, 0.0f, -3.0f, 1.0f);
	const XMVECTOR focusPoint = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	const XMVECTOR upDirection = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	m_viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Update proj matrix
	float aspectRatio = m_window->GetClientWidth() / static_cast<float>(m_window->GetClientHeight());
	m_projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_fov), aspectRatio, 0.1f, 100.0f);
}

void b2Game::OnRender(RenderEventArgs& e)
{
	Game::OnRender(e);

	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto commandList = commandQueue->GetCommandList();

	// Cleat the render targets
	{
		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		commandList->ClearTexture(m_gbufferRT.GetTexture(AttachmentPoint::Color0), clearColor);
		commandList->ClearTexture(m_gbufferRT.GetTexture(AttachmentPoint::Color1), clearColor);
		commandList->ClearTexture(m_gbufferRT.GetTexture(AttachmentPoint::Color2), clearColor);
		commandList->ClearDepthStencilTexture(m_gbufferRT.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);

		commandList->ClearTexture(m_hdrRT.GetTexture(AttachmentPoint::Color0), clearColor);
		commandList->ClearDepthStencilTexture(m_hdrRT.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
	}

	commandList->SetScissorRect(m_scissorRect);

	// Render PBS objects
	{
		commandList->SetRenderTarget(m_gbufferRT);
		commandList->SetViewport(m_gbufferRT.GetViewport());

		// Update the MVP matrix
		XMMATRIX mvpMatrix = XMMatrixMultiply(m_modelMatrix, m_viewMatrix);
		mvpMatrix = XMMatrixMultiply(mvpMatrix, m_projectionMatrix);

		Mat matrices;
		matrices.ModelViewProjectionMatrix = mvpMatrix;

		commandList->SetPipelineState(m_pbsObjectPSO);
		commandList->SetGraphicsRootSignature(m_pbsObjectSig);

		//commandList->SetShaderResourceView(PBSObjectParameters::Textures, 0, m_baseColorTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(PBSObjectParameters::Textures, 0, m_checker2Tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(PBSObjectParameters::Textures, 1, m_normalTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(PBSObjectParameters::Textures, 2, m_metalTex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandList->SetGraphicsDynamicConstantBuffer(PBSObjectParameters::MatricesCB, matrices);

		m_cubeMesh->Draw(*commandList);
	}

	// Resolve Lighting
	{
		commandList->SetRenderTarget(m_hdrRT);
		commandList->SetViewport(m_hdrRT.GetViewport());
		
		commandList->SetPipelineState(m_lightingPSO);
		commandList->SetGraphicsRootSignature(m_lightingSig);
		
		commandList->SetShaderResourceView(LightingParameters::GBuffer, 0, m_gbufferRT.GetTexture(AttachmentPoint::Color0), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(LightingParameters::GBuffer, 1, m_gbufferRT.GetTexture(AttachmentPoint::Color1), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(LightingParameters::GBuffer, 2, m_gbufferRT.GetTexture(AttachmentPoint::Color2), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		
		m_fsQuad->Draw(*commandList);
	}

	// Resolve Lighting
	{
		const auto& windowRT = m_window->GetRenderTarget();
		commandList->SetRenderTarget(windowRT);
		commandList->SetViewport(windowRT.GetViewport());
	
		commandList->SetPipelineState(m_hdr2sdrPSO);
		commandList->SetGraphicsRootSignature(m_hdr2sdrSig);
	
		commandList->SetShaderResourceView(Hdr2SdrParameters::HDR, 0, m_hdrRT.GetTexture(AttachmentPoint::Color0), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
		m_fsQuad->Draw(*commandList);
	}

	commandQueue->ExecuteCommandList(commandList);

	OnGui();


	//std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(10));

	//m_window->Present(m_gbufferRT.GetTexture(AttachmentPoint::Color0));
	m_window->Present();
}

void b2Game::OnKeyPressed(KeyEventArgs& e)
{
	Game::OnKeyPressed(e);

	switch (e.Key)
	{
	case KeyCode::Escape:
		Application::Get().Quit(0);
		break;
	case KeyCode::Enter:
		if (e.Alt)
		{
	case KeyCode::F11:
		m_window->ToggleFullscreen();
		break;
		}
		break;
	case KeyCode::V:
	{
		m_window->ToggleVSync();
	}
	break;
	}
}

void b2Game::OnMouseWheel(MouseWheelEventArgs& e)
{
	m_fov -= e.WheelDelta;
	m_fov = std::clamp(m_fov, 12.0f, 90.0f);

	char buffer[256];
	sprintf_s(buffer, "FOV: %f\n", m_fov);
	OutputDebugStringA(buffer);
}

void b2Game::OnResize(ResizeEventArgs& e)
{
	Game::OnResize(e);

	if (m_width != e.Width || m_height != e.Height)
	{
		m_width = std::max(1, e.Width);
		m_height = std::max(1, e.Height);

		m_hdrRT.Resize(m_width, m_height);
		m_gbufferRT.Resize(m_width, m_height);
	}
}

void b2Game::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);

	commandList->ResourceBarrier(1, &barrier);
}

void b2Game::UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	ID3D12Resource** destinationResource, ID3D12Resource** intermediateResource,
	size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags /*= D3D12_RESOURCE_FLAG_NONE*/)
{
	auto device = Application::Get().GetDevice();

	size_t bufferSize = numElements * elementSize;

	// Create a committed resource for the GPU resource in a default heap
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(destinationResource)
	));

	if (bufferData)
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(intermediateResource)
		));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(commandList.Get(), *destinationResource, *intermediateResource, 0, 0, 1, &subresourceData);
	}
}

void b2Game::OnGui()
{
	//static bool showDemoWindow = false;
	//static bool showOptions = true;

	//if (ImGui::BeginMainMenuBar())
	//{
	//	if (ImGui::BeginMenu("File"))
	//	{
	//		if (ImGui::MenuItem("Exit", "Esc"))
	//		{
	//			Application::Get().Quit();
	//		}
	//		ImGui::EndMenu();
	//	}

	//	if (ImGui::BeginMenu("View"))
	//	{
	//		ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow);
	//		ImGui::MenuItem("Tonemapping", nullptr, &showOptions);

	//		ImGui::EndMenu();
	//	}

		//	if (ImGui::BeginMenu("Options"))
		//	{
		//		bool vSync = m_pWindow->IsVSync();
		//		if (ImGui::MenuItem("V-Sync", "V", &vSync))
		//		{
		//			m_pWindow->SetVSync(vSync);
		//		}

		//		bool fullscreen = m_pWindow->IsFullScreen();
		//		if (ImGui::MenuItem("Full screen", "Alt+Enter", &fullscreen))
		//		{
		//			m_pWindow->SetFullscreen(fullscreen);
		//		}

		//		ImGui::EndMenu();
		//	}

		//	char buffer[256];

		//	{
		//		// Output a slider to scale the resolution of the HDR render target.
		//		float renderScale = m_RenderScale;
		//		ImGui::PushItemWidth(300.0f);
		//		ImGui::SliderFloat("Resolution Scale", &renderScale, 0.1f, 2.0f);
		//		// Using Ctrl+Click on the slider, the user can set values outside of the 
		//		// specified range. Make sure to clamp to a sane range to avoid creating
		//		// giant render targets.
		//		renderScale = clamp(renderScale, 0.0f, 2.0f);

		//		// Output current resolution of render target.
		//		auto size = m_HDRRenderTarget.GetSize();
		//		ImGui::SameLine();
		//		sprintf_s(buffer, _countof(buffer), "(%ux%u)", size.x, size.y);
		//		ImGui::Text(buffer);

		//		// Resize HDR render target if the scale changed.
		//		if (renderScale != m_RenderScale)
		//		{
		//			m_RenderScale = renderScale;
		//			RescaleHDRRenderTarget(m_RenderScale);
		//		}
		//	}

		//char buffer[256];
		//
		//{
		//	sprintf_s(buffer, _countof(buffer), "FPS: %.2f (%.2f ms) Frame: %i  ", g_FPS, 1.0 / g_FPS * 1000.0, Application::GetFrameCount());
		//	auto fpsTextSize = ImGui::CalcTextSize(buffer);
		//	ImGui::SameLine(ImGui::GetWindowWidth() - fpsTextSize.x);
		//	ImGui::Text(buffer);
		//}

		//ImGui::EndMainMenuBar();
	//}

	//if (showDemoWindow)
	//{
	bool t = true;
		ImGui::ShowDemoWindow(&t);
	//}

	//if (showOptions)
	//{
	//	ImGui::Begin("Tonemapping", &showOptions);
	//	{
	//		ImGui::TextWrapped("Use the Exposure slider to adjust the overall exposure of the HDR scene.");
	//		ImGui::SliderFloat("Exposure", &g_TonemapParameters.Exposure, -10.0f, 10.0f);
	//		ImGui::SameLine(); ShowHelpMarker("Adjust the overall exposure of the HDR scene.");
	//		ImGui::SliderFloat("Gamma", &g_TonemapParameters.Gamma, 0.01f, 5.0f);
	//		ImGui::SameLine(); ShowHelpMarker("Adjust the Gamma of the output image.");

	//		const char* toneMappingMethods[] = {
	//			"Linear",
	//			"Reinhard",
	//			"Reinhard Squared",
	//			"ACES Filmic"
	//		};

	//		ImGui::Combo("Tonemapping Methods", (int*)(&g_TonemapParameters.TonemapMethod), toneMappingMethods, 4);

	//		switch (g_TonemapParameters.TonemapMethod)
	//		{
	//		case TonemapMethod::TM_Linear:
	//			ImGui::PlotLines("Linear Tonemapping", &LinearTonemappingPlot, nullptr, VALUES_COUNT, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 250));
	//			ImGui::SliderFloat("Max Brightness", &g_TonemapParameters.MaxLuminance, 1.0f, 10.0f);
	//			ImGui::SameLine(); ShowHelpMarker("Linearly scale the HDR image by the maximum brightness.");
	//			break;
	//		case TonemapMethod::TM_Reinhard:
	//			ImGui::PlotLines("Reinhard Tonemapping", &ReinhardTonemappingPlot, nullptr, VALUES_COUNT, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 250));
	//			ImGui::SliderFloat("Reinhard Constant", &g_TonemapParameters.K, 0.01f, 10.0f);
	//			ImGui::SameLine(); ShowHelpMarker("The Reinhard constant is used in the denominator.");
	//			break;
	//		case TonemapMethod::TM_ReinhardSq:
	//			ImGui::PlotLines("Reinhard Squared Tonemapping", &ReinhardSqrTonemappingPlot, nullptr, VALUES_COUNT, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 250));
	//			ImGui::SliderFloat("Reinhard Constant", &g_TonemapParameters.K, 0.01f, 10.0f);
	//			ImGui::SameLine(); ShowHelpMarker("The Reinhard constant is used in the denominator.");
	//			break;
	//		case TonemapMethod::TM_ACESFilmic:
	//			ImGui::PlotLines("ACES Filmic Tonemapping", &ACESFilmicTonemappingPlot, nullptr, VALUES_COUNT, 0, nullptr, 0.0f, 1.0f, ImVec2(0, 250));
	//			ImGui::SliderFloat("Shoulder Strength", &g_TonemapParameters.A, 0.01f, 5.0f);
	//			ImGui::SliderFloat("Linear Strength", &g_TonemapParameters.B, 0.0f, 100.0f);
	//			ImGui::SliderFloat("Linear Angle", &g_TonemapParameters.C, 0.0f, 1.0f);
	//			ImGui::SliderFloat("Toe Strength", &g_TonemapParameters.D, 0.01f, 1.0f);
	//			ImGui::SliderFloat("Toe Numerator", &g_TonemapParameters.E, 0.0f, 10.0f);
	//			ImGui::SliderFloat("Toe Denominator", &g_TonemapParameters.F, 1.0f, 10.0f);
	//			ImGui::SliderFloat("Linear White", &g_TonemapParameters.LinearWhite, 1.0f, 120.0f);
	//			break;
	//		default:
	//			break;
	//		}
	//	}

	//	if (ImGui::Button("Reset to Defaults"))
	//	{
	//		TonemapMethod method = g_TonemapParameters.TonemapMethod;
	//		g_TonemapParameters = TonemapParameters();
	//		g_TonemapParameters.TonemapMethod = method;
	//	}

	//	ImGui::End();

	//}
}

bool b2Game::LoadContent()
{
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	m_fsQuad = Mesh::FullScreenQuad(*commandList);

	m_cubeMesh = Mesh::LoadFBXFromFile(*commandList, L"Resources/Models/Cube.fbx");
	//m_cubeMesh = Mesh::LoadFBXFromFile(*commandList, L"Resources/Models/Sphere.fbx");
	//m_cubeMesh = Mesh::LoadFBXFromFile(*commandList, L"Resources/Models/Plane.fbx");

	//commandList->LoadFromTextureFile(m_b3nderTexture, L"Resources/Textures/b3nder.tga");
	commandList->LoadFromTextureFile(m_baseColorTex, L"Resources/Textures/metal_base.dds");
	commandList->LoadFromTextureFile(m_normalTex, L"Resources/Textures/metal_normal.dds");
	commandList->LoadFromTextureFile(m_metalTex, L"Resources/Textures/metal_mero.dds");

	commandList->LoadFromTextureFile(m_checkerTex, L"Resources/Textures/Checker.png");
	commandList->LoadFromTextureFile(m_checker2Tex, L"Resources/Textures/Checker2.png");

	// Check is root signature version 1.1 is available.
	// Version 1.1 is preferred over 1.0 because it allows GPU to optimize some stuff
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Depth
	DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT;
	auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthFormat, m_width, m_height);
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClearValue;
	depthClearValue.Format = depthDesc.Format;
	depthClearValue.DepthStencil = { 1.0f, 0 };

	Texture depthTexture = Texture(depthDesc, &depthClearValue, TextureUsage::Depth, L"Depth");

	// Create a G-Buffer
	{
		DXGI_FORMAT gbufferColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(gbufferColorFormat, m_width, m_height, 1, 1);
		colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = colorDesc.Format;
		colorClearValue.Color[0] = 0.4f;
		colorClearValue.Color[1] = 0.6f;
		colorClearValue.Color[2] = 0.9f;
		colorClearValue.Color[3] = 1.0f;

		Texture gb0 = Texture{ colorDesc, &colorClearValue, TextureUsage::RenderTarget, L"G-Buffer 0" }; // Base color
		Texture gb1 = Texture{ colorDesc, &colorClearValue, TextureUsage::RenderTarget, L"G-Buffer 1" }; // Normal
		Texture gb2 = Texture{ colorDesc, &colorClearValue, TextureUsage::RenderTarget, L"G-Buffer 2" }; // MetalRoughness

		D3D12_CLEAR_VALUE depthClearValue;
		depthClearValue.Format = depthDesc.Format;
		depthClearValue.DepthStencil = { 1.0f, 0 };

		m_gbufferRT.AttachTexture(AttachmentPoint::Color0, gb0);
		m_gbufferRT.AttachTexture(AttachmentPoint::Color1, gb1);
		m_gbufferRT.AttachTexture(AttachmentPoint::Color2, gb2);
		m_gbufferRT.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
	}

	// Create an HDR RT
	{
		DXGI_FORMAT hdrFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

		auto colorDesc = CD3DX12_RESOURCE_DESC::Tex2D(hdrFormat, m_width, m_height, 1, 1);
		colorDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = colorDesc.Format;
		colorClearValue.Color[0] = 0.4f;
		colorClearValue.Color[1] = 0.6f;
		colorClearValue.Color[2] = 0.9f;
		colorClearValue.Color[3] = 1.0f;

		Texture hdrTexture = Texture{ colorDesc, &colorClearValue, TextureUsage::RenderTarget, L"HDR RT" };

		m_hdrRT.AttachTexture(AttachmentPoint::Color0, hdrTexture);
		m_hdrRT.AttachTexture(AttachmentPoint::DepthStencil, depthTexture);
	}

	// Root signature and PSO for PBS objects. Objects -> GBuffer
	{
		// Load the vertex shader
		ComPtr<ID3DBlob> vertexShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PBS_object_vs.cso", &vertexShaderBlob));

		// Load the pixel shader
		ComPtr<ID3DBlob> pixelShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PBS_object_ps.cso", &pixelShaderBlob));

		// Allow input layout and deny unnecessary access to certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_DESCRIPTOR_RANGE1 descriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);

		CD3DX12_ROOT_PARAMETER1 rootParameters[PBSObjectParameters::NumPBSObjectParameters];
		rootParameters[PBSObjectParameters::MatricesCB].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[PBSObjectParameters::Textures].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
		rootSignatureDescription.Init_1_1(PBSObjectParameters::NumPBSObjectParameters, rootParameters, 1, &anisotropicSampler, rootSignatureFlags);

		m_pbsObjectSig.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

		struct PipelineStateStream
		{
			CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE rootSignature;
			CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
			CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
			CD3DX12_PIPELINE_STATE_STREAM_VS vs;
			CD3DX12_PIPELINE_STATE_STREAM_PS ps;
			CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormat;
			CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormats;
		} pipelineStateStream;

		pipelineStateStream.rootSignature = m_pbsObjectSig.GetRootSignature().Get();
		pipelineStateStream.inputLayout = { VertexPositionDef::InputElements, VertexPositionDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.rtvFormats = m_gbufferRT.GetRenderTargetFormats();
		pipelineStateStream.dsvFormat = m_gbufferRT.GetDepthStencilFormat();

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
		{
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pbsObjectPSO)));
	}

	// Root signature and PSO for Lighting. GBuffer -> HDR
	{
		// Load the vertex shader
		ComPtr<ID3DBlob> vertexShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Lighting_vs.cso", &vertexShaderBlob));

		// Load the pixel shader
		ComPtr<ID3DBlob> pixelShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Lighting_ps.cso", &pixelShaderBlob));

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

		m_lightingSig.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

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
		pipelineStateStream.inputLayout = { VertexPositionDef::InputElements, VertexPositionDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.rtvFormats = m_hdrRT.GetRenderTargetFormats();

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
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Hdr2Sdr_vs.cso", &vertexShaderBlob));

		// Load the pixel shader
		ComPtr<ID3DBlob> pixelShaderBlob;
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/Hdr2Sdr_ps.cso", &pixelShaderBlob));

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

		m_hdr2sdrSig.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

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
		pipelineStateStream.inputLayout = { VertexPositionDef::InputElements, VertexPositionDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.Rasterizer = rasterizerDesc;
		pipelineStateStream.rtvFormats = m_window->GetRenderTarget().GetRenderTargetFormats();

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
		{
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_hdr2sdrPSO)));
	}

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);

	return true;
}

void b2Game::UnloadContent()
{
}