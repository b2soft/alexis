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

SampleApp::SampleApp() :
	m_viewport(0.0f, 0.0f, static_cast<float>(alexis::g_clientWidth), static_cast<float>(alexis::g_clientHeight)),
	m_scissorRect(0, 0, static_cast<LONG>(alexis::g_clientWidth), static_cast<LONG>(alexis::g_clientHeight))
{
	m_aspectRatio = static_cast<float>(alexis::g_clientWidth) / static_cast<float>(alexis::g_clientHeight);
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

	bool show = true;
	ImGui::ShowDemoWindow(&show);

	m_frameCount++;
	m_timeCount += dt;

	if (m_timeCount > 1.f)
	{
		m_fps = m_frameCount;
		m_timeCount = 0.f;
		m_frameCount = 0;

		std::wstring fpsString = L"FPS: " + std::to_wstring(m_fps) + L"\n";
		OutputDebugString(fpsString.c_str());
	}

	const float translationSpeed = 1.f;
	const float offsetBounds = 1.25f;

	// Update the model matrix
	float angle = 45.0;
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
	auto render = alexis::Render::GetInstance();
	auto device = render->GetDevice();

	// Create empty root signature
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}

		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

		// Sampler
		CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

		// Allow input layout and deny unnecessary access to certain pipeline stages
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &anisotropicSampler, rootSignatureFlags);

		m_rootSignature.SetRootSignatureDesc(rootSignatureDesc.Desc_1_1, featureData.HighestVersion);
	}

	// Create PSO + loading shaders + compiling shaders
	{
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/VertexShader_d.cso", &vertexShader));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PixelShader_d.cso", &pixelShader));
#else
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/VertexShader.cso", &vertexShader));
		ThrowIfFailed(D3DReadFileToBlob(L"Resources/Shaders/PixelShader.cso", &pixelShader));
#endif

		// Create PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { VertexDef::InputElements, VertexDef::InputElementCount };
		psoDesc.pRootSignature = m_rootSignature.GetRootSignature().Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
	}

	//----- Loading
	auto commandManager = CommandManager::GetInstance();
	auto commandContext = commandManager->CreateCommandContext();
	auto commandList = commandContext->List.Get();
	//------

	// Constant Buffer

	m_triangleCB.Create(1, sizeof(Mat));
	//commandContext->CopyBuffer(m_triangleCB, &m_constantBufferData, 1, sizeof(SceneConstantBuffer));
	//commandContext->TransitionResource(m_triangleCB, D3D12_RESOURCE_STATE_GENERIC_READ, true, D3D12_RESOURCE_STATE_COPY_DEST);

	commandContext->LoadTextureFromFile(m_checkerTexture, L"Resources/Textures/Checker2.dds");
	commandContext->TransitionResource(m_checkerTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true, D3D12_RESOURCE_STATE_COPY_DEST);

	m_cubeMesh = Mesh::LoadFBXFromFile(commandContext, L"Resources/Models/Cube.fbx");
	commandManager->ExecuteCommandContext(commandContext, true);


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

	// Set necessary states

	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	commandList->SetPipelineState(m_pipelineState.Get());
	commandContext->SetRootSignature(m_rootSignature);

	auto backbufferResource = render->GetCurrentBackBufferResource();
	auto backBufferRTV = render->GetCurrentBackBufferRTV();
	//// Transition back buffer -> RT
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backbufferResource, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	commandList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);

	// Record actual commands
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(backBufferRTV, clearColor, 0, nullptr);

	// Update the MVP matrix
	XMMATRIX mvpMatrix = XMMatrixMultiply(m_modelMatrix, m_viewMatrix);
	mvpMatrix = XMMatrixMultiply(mvpMatrix, m_projectionMatrix);

	Mat matrices;
	matrices.ModelViewProjectionMatrix = mvpMatrix;

	memcpy(m_triangleCB.GetCPUPtr(), &matrices, sizeof(Mat));
	commandContext->SetDynamicCBV(0, m_triangleCB);
	commandContext->SetSRV(1, 0, m_checkerTexture);

	m_cubeMesh->Draw(commandContext);


	// GUI
	ID3D12DescriptorHeap* imguiHeap[] = { m_imguiSrvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(imguiHeap), imguiHeap);
	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

	// Transit back buffer back to Present state

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backbufferResource, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	commandManager->ExecuteCommandContext(commandContext);
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
