#include "Precompiled.h"

#include "SampleApp.h"

#include <CoreHelpers.h>
#include <d3dcompiler.h>

#include <Core/Core.h>
#include <Core/Render.h>
#include <Core/CommandManager.h>

#include <imgui.h>
#include <imgui_impl_dx12.h>
#include <imgui_impl_win32.h>

using namespace alexis;
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

SampleApp::SampleApp() :
	m_viewport(0.0f, 0.0f, static_cast<float>(alexis::g_clientWidth), static_cast<float>(alexis::g_clientHeight)),
	m_scissorRect(0, 0, LONG_MAX, LONG_MAX),
	m_aspectRatio(static_cast<float>(alexis::g_clientWidth) / static_cast<float>(alexis::g_clientHeight))
{
}

bool SampleApp::Initialize()
{
	return true;
}

void SampleApp::OnUpdate(float dt)
{
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
		m_fps = m_frameCount;
		m_timeCount = 0.f;
		m_frameCount = 0;
	}

	const float translationSpeed = 1.f;
	const float offsetBounds = 1.25f;

	// Update the model matrix
	float angle = static_cast<float>(m_totalTime * 90.0);
	const XMVECTOR rotationAxis = XMVectorSet(1.0f, 1.0f, 0.0f, 0.0f);
	m_modelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

	// Update view matrix
	const XMVECTOR eyePosition = XMVectorSet(0.0f, 0.0f, -3.0f, 1.0f);
	const XMVECTOR focusPoint = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	const XMVECTOR upDirection = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
	m_viewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Update proj matrix
	m_projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(75.0f), m_aspectRatio, 0.1f, 100.0f);
}

void SampleApp::OnRender()
{
	// Records commands
	PopulateCommandList();
}

void SampleApp::OnResize(int width, int height)
{
	m_gbufferRT.Resize(width, height);
	m_hdrRT.Resize(width, height);

	m_aspectRatio = static_cast<float>(alexis::g_clientWidth) / static_cast<float>(alexis::g_clientHeight);
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
	auto commandManager = CommandManager::GetInstance();
	auto commandContext = commandManager->CreateCommandContext();
	auto commandList = commandContext->List.Get();
	//------

	// FS Quad
	m_fsQuad = Mesh::FullScreenQuad(commandContext);

	// Constant Buffer

	m_triangleCB.Create(1, sizeof(Mat));
	//commandContext->CopyBuffer(m_triangleCB, &m_constantBufferData, 1, sizeof(SceneConstantBuffer));
	//commandContext->TransitionResource(m_triangleCB, D3D12_RESOURCE_STATE_GENERIC_READ, true, D3D12_RESOURCE_STATE_COPY_DEST);

	// Textures
	commandContext->LoadTextureFromFile(m_checkerTexture, L"Resources/Textures/Checker2.dds");
	commandContext->TransitionResource(m_checkerTexture, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandContext->LoadTextureFromFile(m_concreteTex, L"Resources/Textures/metal_base.dds");
	commandContext->TransitionResource(m_concreteTex, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandContext->LoadTextureFromFile(m_normalTex, L"Resources/Textures/metal_normal.dds");
	commandContext->TransitionResource(m_normalTex, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	commandContext->LoadTextureFromFile(m_metalTex, L"Resources/Textures/metal_mero.dds");
	commandContext->TransitionResource(m_metalTex, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Models 
	m_cubeMesh = Mesh::LoadFBXFromFile(commandContext, L"Resources/Models/Cube.fbx");
	commandManager->ExecuteCommandContext(commandContext, true);


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

		TextureBuffer gb1;
		gb1.Create(colorDesc, &colorClearValue); // Base color
		m_gbufferRT.AttachTexture(gb1, RenderTarget::Slot::Slot1);

		TextureBuffer gb2;
		gb2.Create(colorDesc, &colorClearValue); // Base color
		m_gbufferRT.AttachTexture(gb2, RenderTarget::Slot::Slot2);

		m_gbufferRT.AttachTexture(depthTexture, RenderTarget::Slot::DepthStencil);
	}

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

		m_hdrRT.AttachTexture(hdrTexture, RenderTarget::Slot::Slot0);
		m_hdrRT.AttachTexture(depthTexture, RenderTarget::Slot::DepthStencil);
	}

	// Root signature and PSO for PBS objects. Objects -> GBuffer
	{
		ComPtr<ID3DBlob> vertexShaderBlob;
		ComPtr<ID3DBlob> pixelShaderBlob;
#if defined(_DEBUG)
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PBS_object_vs_d.cso", &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PBS_object_ps_d.cso", &pixelShaderBlob));
#else
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PBS_object_vs.cso", &vertexShaderBlob));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PBS_object_ps.cso", &pixelShaderBlob));
#endif

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
		pipelineStateStream.inputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
		pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
		pipelineStateStream.rtvFormats = m_gbufferRT.GetFormat();
		pipelineStateStream.dsvFormat = m_gbufferRT.GetDSFormat();

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc =
		{
			sizeof(PipelineStateStream), &pipelineStateStream
		};

		ThrowIfFailed(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pbsObjectPSO)));
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


	IMGUI_CHECKVERSION();
	m_context = ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(Core::GetHwnd());
	ImGui_ImplDX12_Init(Render::GetInstance()->GetDevice(), 2,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		m_imguiSrvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_imguiSrvHeap->GetGPUDescriptorHandleForHeapStart());


	// Create Synchronization Objects
	{
		alexis::CommandManager::GetInstance()->WaitForGpu();
	}
}

void SampleApp::PopulateCommandList()
{
	auto render = alexis::Render::GetInstance();
	auto commandManager = alexis::CommandManager::GetInstance();
	auto commandContext = commandManager->CreateCommandContext();
	auto commandList = commandContext->List.Get();

	// Clear G-Buffer and HDR RT
	{
		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		auto& gbDepthStencil = m_gbufferRT.GetTexture(RenderTarget::DepthStencil);

		for (int i = RenderTarget::Slot::Slot0; i < RenderTarget::Slot::DepthStencil; ++i)
		{
			auto& texture = m_gbufferRT.GetTexture(static_cast<RenderTarget::Slot>(i));
			if (texture.IsValid())
			{
				commandContext->TransitionResource(texture, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
				commandContext->ClearTexture(texture, clearColor);
			}
		}

		commandContext->TransitionResource(gbDepthStencil, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		commandContext->ClearDepthStencil(gbDepthStencil, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0);

		commandContext->TransitionResource(m_hdrRT.GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandContext->ClearTexture(m_hdrRT.GetTexture(RenderTarget::Slot::Slot0), clearColor);
	}

	commandList->RSSetScissorRects(1, &m_scissorRect);

	// Render PBS objects
	{
		commandContext->SetRenderTarget(m_gbufferRT);
		commandContext->SetViewport(m_gbufferRT.GetViewport());

		// Update the MVP matrix
		XMMATRIX mvpMatrix = XMMatrixMultiply(m_modelMatrix, m_viewMatrix);
		mvpMatrix = XMMatrixMultiply(mvpMatrix, m_projectionMatrix);

		Mat matrices;
		matrices.ModelViewProjectionMatrix = mvpMatrix;
		memcpy(m_triangleCB.GetCPUPtr(), &matrices, sizeof(Mat));

		commandList->SetPipelineState(m_pbsObjectPSO.Get());
		commandContext->SetRootSignature(m_pbsObjectSig);

		commandContext->SetSRV(PBSObjectParameters::Textures, 0, m_checkerTexture);
		commandContext->SetSRV(PBSObjectParameters::Textures, 1, m_normalTex);
		commandContext->SetSRV(PBSObjectParameters::Textures, 2, m_metalTex);

		commandContext->SetDynamicCBV(0, m_triangleCB);

		m_cubeMesh->Draw(commandContext);
	}

	// Resolve lighting
	{
		commandContext->TransitionResource(m_gbufferRT.GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandContext->TransitionResource(m_gbufferRT.GetTexture(RenderTarget::Slot::Slot1), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandContext->TransitionResource(m_gbufferRT.GetTexture(RenderTarget::Slot::Slot2), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandContext->SetRenderTarget(m_hdrRT);
		commandContext->SetViewport(m_hdrRT.GetViewport());

		commandList->SetPipelineState(m_lightingPSO.Get());
		commandContext->SetRootSignature(m_lightingSig);

		commandContext->SetSRV(LightingParameters::GBuffer, 0, m_gbufferRT.GetTexture(RenderTarget::Slot::Slot0));
		commandContext->SetSRV(LightingParameters::GBuffer, 1, m_gbufferRT.GetTexture(RenderTarget::Slot::Slot1));
		commandContext->SetSRV(LightingParameters::GBuffer, 2, m_gbufferRT.GetTexture(RenderTarget::Slot::Slot2));

		m_fsQuad->Draw(commandContext);
	}

	// Setup render to backbuffer
	const auto& backbuffer = render->GetBackbufferRT();
	const auto& backTexture = backbuffer.GetTexture(RenderTarget::Slot::Slot0);
	commandContext->TransitionResource(backTexture, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Hdr to Sdr conversion
	{
		commandContext->TransitionResource(m_hdrRT.GetTexture(RenderTarget::Slot::Slot0), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		commandContext->SetRenderTarget(backbuffer);
		commandContext->SetViewport(backbuffer.GetViewport());

		commandList->SetPipelineState(m_hdr2sdrPSO.Get());
		commandContext->SetRootSignature(m_hdr2sdrSig);

		commandContext->SetSRV(Hdr2SdrParameters::HDR, 0, m_hdrRT.GetTexture(RenderTarget::Slot::Slot0));

		m_fsQuad->Draw(commandContext);
	}

	// GUI
	ID3D12DescriptorHeap* imguiHeap[] = { m_imguiSrvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(imguiHeap), imguiHeap);
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	// Transit back buffer back to Present state

	commandContext->TransitionResource(backTexture, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	commandManager->ExecuteCommandContext(commandContext);
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
			sprintf_s(buffer, _countof(buffer), "FPS: %i (%.2f ms) Frame: %I64i  ", m_fps, 1.0 / m_fps * 1000.0, alexis::Core::GetFrameCount());
			auto fpsTextSize = ImGui::CalcTextSize(buffer);
			ImGui::SameLine(ImGui::GetWindowWidth() - fpsTextSize.x);
			ImGui::Text(buffer);
		}

		ImGui::EndMainMenuBar();
	}

	if (showDemoWindow)
	{
		ImGui::ShowDemoWindow(&showDemoWindow);
	}
}

bool SampleApp::LoadContent()
{
	LoadPipeline();
	LoadAssets();

	return true;
}

void SampleApp::UnloadContent()
{

}
