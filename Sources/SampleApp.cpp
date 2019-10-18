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

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT2 uv;
};

SampleApp::SampleApp() :
	m_viewport(0.0f, 0.0f, static_cast<float>(alexis::g_clientWidth), static_cast<float>(alexis::g_clientHeight)),
	m_scissorRect(0, 0, static_cast<LONG>(alexis::g_clientWidth), static_cast<LONG>(alexis::g_clientHeight))//,
	//m_constantBufferData{}
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

	m_constantBufferData.offset.x += translationSpeed * dt;
	if (m_constantBufferData.offset.x > offsetBounds)
	{
		m_constantBufferData.offset.x = -offsetBounds;
	}

	memcpy(m_triangleCB.GetCPUPtr(), &m_constantBufferData, sizeof(m_constantBufferData));
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

	// Create Bundle Allocator
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&m_bundleAllocator)));
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
		//ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

		CD3DX12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE, D3D12_SHADER_VISIBILITY_VERTEX);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		

		// Sampler
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

		D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Create PSO
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDesc, _countof(inputElementDesc) };
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

	// Create Vertex Buffer

	Vertex triangleVertices[] =
	{
		{ {0.0f, 0.25f * m_aspectRatio, 0.0f}, {0.5f, 0.0f,} },
		{ {0.25f, -0.25f * m_aspectRatio, 0.0f}, {1.0f, 1.0f} },
		{ {-0.25f, -0.25f * m_aspectRatio, 0.0f}, {0.0, 1.0f} },
	};

	const UINT vertexBufferSize = sizeof(triangleVertices);

	m_triangleVB.Create(3, sizeof(Vertex));
	commandContext->CopyBuffer(m_triangleVB, &triangleVertices, 3, sizeof(Vertex));
	commandContext->TransitionResource(m_triangleVB, D3D12_RESOURCE_STATE_GENERIC_READ, true, D3D12_RESOURCE_STATE_COPY_DEST);

	// Constant Buffer

	m_triangleCB.Create(1, sizeof(SceneConstantBuffer));
	//commandContext->CopyBuffer(m_triangleCB, &m_constantBufferData, 1, sizeof(SceneConstantBuffer));
	//commandContext->TransitionResource(m_triangleCB, D3D12_RESOURCE_STATE_GENERIC_READ, true, D3D12_RESOURCE_STATE_COPY_DEST);

	// Create Bundle
	{
		//ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, m_bundleAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_bundle)));
		//m_bundle->SetGraphicsRootSignature(m_rootSignature.Get());
		//m_bundle->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//m_bundle->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		//m_bundle->DrawInstanced(3, 1, 0, 0);
		//ThrowIfFailed(m_bundle->Close());
	}

	commandContext->LoadTextureFromFile(m_checkerTexture, L"Resources/Textures/Checker2.png");
	commandContext->TransitionResource(m_checkerTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true, D3D12_RESOURCE_STATE_COPY_DEST);
	commandManager->ExecuteCommandContext(commandContext, true);

	//std::vector<UINT8> texture = GenerateTextureData();

	//D3D12_SUBRESOURCE_DATA textureData = {};
	//textureData.pData = &texture[0];
	//textureData.RowPitch = k_textureSize * k_texturePixelSize;
	//textureData.SlicePitch = textureData.RowPitch * k_textureSize;

	//m_checkerTexture.Create(k_textureSize, k_textureSize, DXGI_FORMAT_R8G8B8A8_UNORM, 1);
	//commandContext->InitializeTexture(m_checkerTexture, 1, &textureData);
	//commandContext->TransitionResource(m_checkerTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true, D3D12_RESOURCE_STATE_COPY_DEST);

	//commandManager->ExecuteCommandContext(commandContext, true);

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

std::vector<UINT8> SampleApp::GenerateTextureData()
{
	const UINT rowPitch = k_textureSize * k_texturePixelSize;
	const UINT cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
	const UINT cellHeight = k_textureSize >> 3;    // The height of a cell in the checkerboard texture.
	const UINT textureSize = rowPitch * k_textureSize;

	std::vector<UINT8> data(textureSize);
	UINT8* pData = &data[0];

	for (UINT n = 0; n < textureSize; n += k_texturePixelSize)
	{
		UINT x = n % rowPitch;
		UINT y = n / rowPitch;
		UINT i = x / cellPitch;
		UINT j = y / cellHeight;

		if (i % 2 == j % 2)
		{
			pData[n] = 0x00;        // R
			pData[n + 1] = 0x00;    // G
			pData[n + 2] = 0x00;    // B
			pData[n + 3] = 0xff;    // A
		}
		else
		{
			pData[n] = 0xff;        // R
			pData[n + 1] = 0xff;    // G
			pData[n + 2] = 0xff;    // B
			pData[n + 3] = 0xff;    // A
		}
	}

	return data;
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

	//commandContext->SetCBV(0, 0, m_triangleCB);
	commandContext->SetDynamicCBV(0, m_triangleCB);
	commandContext->SetSRV(1, 0, m_checkerTexture);

	// Record actual commands
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(backBufferRTV, clearColor, 0, nullptr);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_triangleVB.GetVertexBufferView());
	commandContext->DrawInstanced(3, 1, 0, 0);

	//commandList->ExecuteBundle(m_bundle.Get());

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
