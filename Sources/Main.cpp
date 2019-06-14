#include "Precompiled.h"

using namespace DirectX;

bool g_isRunning = true;

// WinAPI Stuff (temporary, will be refactored)

HWND g_hwnd = nullptr;

const LPCTSTR k_windowName = L"Render DX12";
const LPCTSTR k_windowTitle = L"Window";

const int k_width = 800;
const int k_height = 600;

const bool k_fullScreen = false;

// DirectX Stuff (temporary, will be refactored)

const int frameBufferCount = 3; // Triple Buffering
ID3D12Device* device;
IDXGISwapChain3* swapChain; // Defines way to switch between RT
ID3D12CommandQueue* commandQueue; // Container for command lists
ID3D12DescriptorHeap* rtvDescriptorHeap; // Descriptor heap to hold like info about RT
ID3D12Resource* renderTargets[frameBufferCount]; // RTs
ID3D12CommandAllocator* commandAllocator[frameBufferCount]; // Allocators number is frameBuffers * number of threads (1 thread for now)
ID3D12GraphicsCommandList* commandList; // Command list
ID3D12Fence* fence[frameBufferCount]; // Object to be locked while command list is executed on GPU. Need as many, as allocators number
HANDLE fenceEvent; // Event for Fence object unlocking
UINT64 fenceValue[frameBufferCount]; // Incremented each frame, each fence has its own value
int frameIndex; // Current RTV rendered
int rtvDescriptorSize; // Size of the RTV descriptor on the device

ID3D12PipelineState* pipelineStateObject;
ID3D12RootSignature* rootSignature;
D3D12_VIEWPORT viewport;
D3D12_RECT scissorRect;

ID3D12Resource* vertexBuffer;
D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

ID3D12Resource* indexBuffer;
D3D12_INDEX_BUFFER_VIEW indexBufferView;

ID3D12Resource* depthStencilBuffer;
ID3D12DescriptorHeap* dsvDescriptorHeap;

// Constant buffer
struct ConstantBuffer
{
	XMFLOAT4 testColorMultiplier;
};

ID3D12Resource* constantBufferUploadHeap[frameBufferCount];
ID3D12DescriptorHeap* mainDescriptorHeap[frameBufferCount];
ConstantBuffer cbTestColorMultiplierData;
UINT8* cbTestColorMultiplierGPUAddress[frameBufferCount];






struct Vertex
{
	Vertex(float x, float y, float z, float r, float g, float b, float a) :
		pos(x, y, z),
		color(r, g, b, a)
	{

	}
	XMFLOAT3 pos;
	XMFLOAT4 color;
};


bool InitD3D()
{
	HRESULT hr;

	// Create the Device

	IDXGIFactory4* dxgiFactory;
	hr = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr))
	{
		return false;
	}

	IDXGIAdapter1* adapter; // Graphics cards
	int adapterIndex = 0; // Start from device 0
	bool adapterFound = false; // Set to true if we find DX12 device

	// Find first hardware GPU with DX12
	while (dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Go to next adapter, software is not applicable
			adapterIndex++;
			continue;
		}

		// Check for DX 12 compatibility but test-create the fake device
		hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
			adapterFound = true;
			break;
		}

		adapterIndex++;
	}

	if (!adapterFound)
	{
		return false;
	}

	// Create the device
	hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));
	if (FAILED(hr))
	{
		return false;
	}

	// Create the Command Queue
	D3D12_COMMAND_QUEUE_DESC cqDesc = {};
	hr = device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&commandQueue));
	if (FAILED(hr))
	{
		return false;
	}

	// Swap Chain
	DXGI_MODE_DESC backBufferDesc = {};
	backBufferDesc.Width = k_width;
	backBufferDesc.Height = k_height;
	backBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // Format of the back buffer

	// Multi-sampling (disabled)
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;

	// Describe and create Swap Chain
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	swapChainDesc.BufferCount = frameBufferCount;
	swapChainDesc.BufferDesc = backBufferDesc;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.OutputWindow = g_hwnd;
	swapChainDesc.SampleDesc = sampleDesc;
	swapChainDesc.Windowed = !k_fullScreen;

	IDXGISwapChain* tempSwapChain;

	dxgiFactory->CreateSwapChain(commandQueue, &swapChainDesc, &tempSwapChain);

	swapChain = static_cast<IDXGISwapChain3*>(tempSwapChain);

	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// Create RTV (backbuffers) Descriptor Heap

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = frameBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	// Not visible to shades directly
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
	if (FAILED(hr))
	{
		return false;
	}

	// Size of desc in the RTV heap. Needed to increment offsets
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Get a desc handle in desc heap. Handle is like a pointer but not C++ pointer
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create RTV for each buffer
	for (int i = 0; i < frameBufferCount; i++)
	{
		// Get n'th buffer in the swap chain and store it in the n'th position of the ID3D12Resource array
		hr = swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i]));
		if (FAILED(hr))
		{
			return false;
		}
		// "Create" a RTV that binds buffer to RTV handle
		device->CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);

		rtvHandle.Offset(1, rtvDescriptorSize);
	}

	// Create Command Allocators
	for (int i = 0; i < frameBufferCount; i++)
	{
		hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator[i]));
		if (FAILED(hr))
		{
			return false;
		}
	}

	// Create Command Lists
	hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator[0], nullptr, IID_PPV_ARGS(&commandList));
	if (FAILED(hr))
	{
		return false;
	}

	// Create Fence for each buffer
	for (int i = 0; i < frameBufferCount; i++)
	{
		hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence[i]));
		if (FAILED(hr))
		{
			return false;
		}
		fenceValue[i] = 0;
	}

	// Create a handle to a fence event
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr)
	{
		return false;
	}

	// Create Root Signature

	// Create Descriptor Table
	D3D12_DESCRIPTOR_RANGE descriptorTableRanges[1];
	descriptorTableRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	descriptorTableRanges[0].NumDescriptors = 1;
	descriptorTableRanges[0].BaseShaderRegister = 0;
	descriptorTableRanges[0].RegisterSpace = 0;
	descriptorTableRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_DESCRIPTOR_TABLE descriptorTable;
	descriptorTable.NumDescriptorRanges = _countof(descriptorTableRanges);
	descriptorTable.pDescriptorRanges = &descriptorTableRanges[0];

	D3D12_ROOT_PARAMETER rootParameters[1];
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable = descriptorTable;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(
		_countof(rootParameters),
		rootParameters,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
	);

	ID3DBlob* signature;
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	if (FAILED(hr))
	{
		return false;
	}

	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	if (FAILED(hr))
	{
		return false;
	}

	// Create Vertex Shader
	ID3DBlob* vertexShader;
	ID3DBlob* errorBuffer;
	hr = D3DCompileFromFile(L"../Sources/VertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &vertexShader, &errorBuffer);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
		return false;
	}

	// Fill out vertex shader bytecode
	D3D12_SHADER_BYTECODE vertexShaderBytecode = {};
	vertexShaderBytecode.BytecodeLength = vertexShader->GetBufferSize();
	vertexShaderBytecode.pShaderBytecode = vertexShader->GetBufferPointer();

	// Create Pixel Shader
	ID3DBlob* pixelShader;
	hr = D3DCompileFromFile(L"../Sources/PixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &pixelShader, &errorBuffer);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
		return false;
	}

	// Fill out pixel shader bytecode
	D3D12_SHADER_BYTECODE pixelShaderBytecode = {};
	pixelShaderBytecode.BytecodeLength = pixelShader->GetBufferSize();
	pixelShaderBytecode.pShaderBytecode = pixelShader->GetBufferPointer();


	// Create input layout

	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;


	// Create PSO

	// PSO Desc
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = rootSignature;
	psoDesc.VS = vertexShaderBytecode;
	psoDesc.PS = pixelShaderBytecode;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc = sampleDesc;
	psoDesc.SampleMask = 0xffffffff;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.NumRenderTargets = 1;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	// Create PSO
	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateObject));
	if (FAILED(hr))
	{
		return false;
	}


	// Create VBO

	// Triangle
	Vertex vList[] =
	{
		// first quad (closer to camera, blue)
		{ -0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{  0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{ -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
		{  0.5f,  0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },

		// second quad (further from camera, green)
		{ -0.75f,  0.75f,  0.7f, 0.0f, 1.0f, 0.0f, 1.0f },
		{   0.0f,  0.0f, 0.7f, 0.0f, 1.0f, 0.0f, 1.0f },
		{ -0.75f,  0.0f, 0.7f, 0.0f, 1.0f, 0.0f, 1.0f },
		{   0.0f,  0.75f,  0.7f, 0.0f, 1.0f, 0.0f, 1.0f }
	};

	int vBufferSize = sizeof(vList);

	// Create Default Heap or Vertex Buffer
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&vertexBuffer)
	);

	vertexBuffer->SetName(L"Vertex Buffer Resource Heap");

	// Create Upload Heap
	ID3D12Resource* vBufferUploadHeap;
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vBufferUploadHeap)
	);
	vBufferUploadHeap->SetName(L"Vertex Buffer Upload Resource Heap");

	// Store Vertex buffer in upload heap

	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = reinterpret_cast<BYTE*>(vList);
	vertexData.RowPitch = vBufferSize;
	vertexData.SlicePitch = vBufferSize;

	// Upload to GPU
	UpdateSubresources(commandList, vertexBuffer, vBufferUploadHeap, 0, 0, 1, &vertexData);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));


	// Create Index Buffer
	DWORD iList[] = {
		0, 1, 2,
		0, 3, 1
	};

	int iBufferSize = sizeof(iList);

	// Create Default Heap for Index Buffer
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(iBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&indexBuffer)
	);

	indexBuffer->SetName(L"Index Buffer Resource Heap");

	// Create Upload Heap for Index Buffer
	ID3D12Resource* iBufferUploadHeap;
	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&iBufferUploadHeap)
	);

	iBufferUploadHeap->SetName(L"Index Buffer Upload Resource Heap");

	// Store Index buffer in upload heap
	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = reinterpret_cast<BYTE*>(iList);
	indexData.RowPitch = iBufferSize;
	indexData.SlicePitch = iBufferSize;

	// Upload to GPU
	UpdateSubresources(commandList, indexBuffer, iBufferUploadHeap, 0, 0, 1, &indexData);

	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));



	// Create Depth Stencil Buffer

	// Create a descriptor stencil descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap));
	if (FAILED(hr))
	{
		g_isRunning = false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, k_width, k_height, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&depthStencilBuffer)
	);

	hr = device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvDescriptorHeap));
	if (FAILED(hr))
	{
		g_isRunning = false;
	}
	dsvDescriptorHeap->SetName(L"Depth Stencil Resource Heap");

	device->CreateDepthStencilView(depthStencilBuffer, &depthStencilDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Create constant buffer descriptor heap for each frame
	for (int i = 0; i < frameBufferCount; i++)
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mainDescriptorHeap[i]));
		if (FAILED(hr))
		{
			g_isRunning = false;
		}
	}

	// Create constant buffer resource heap

	for (int i = 0; i < frameBufferCount; i++)
	{
		hr = device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(1024 * 64), // Multiple of 64Kb for constant buffers
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&constantBufferUploadHeap[i])
		);
		constantBufferUploadHeap[i]->SetName(L"Constant Buffer Upload Resource Heap");

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = constantBufferUploadHeap[i]->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = (sizeof(ConstantBuffer) + 255) & ~255;
		device->CreateConstantBufferView(&cbvDesc, mainDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart());

		ZeroMemory(&cbTestColorMultiplierData, sizeof(cbTestColorMultiplierData));

		CD3DX12_RANGE readRange(0, 0);
		hr = constantBufferUploadHeap[i]->Map(0, &readRange, reinterpret_cast<void**>(&cbTestColorMultiplierGPUAddress[i]));
		memcpy(cbTestColorMultiplierGPUAddress[i], &cbTestColorMultiplierData, sizeof(cbTestColorMultiplierData));
	}

	commandList->Close();
	ID3D12CommandList* ppCommandLists[] = { commandList };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Increment the fence value, to guarantee that buffer is uploaded
	fenceValue[frameIndex]++;
	hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(hr))
	{
		g_isRunning = false;
	}

	// Create a Vertex Buffer View to get buffer location in GPU memory
	vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vertexBufferView.StrideInBytes = sizeof(Vertex);
	vertexBufferView.SizeInBytes = vBufferSize;

	// Create a Vertex Buffer View to get buffer location in GPU memory
	indexBufferView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexBufferView.SizeInBytes = iBufferSize;

	// Set up Viewport
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = k_width;
	viewport.Height = k_height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Set up Scissor Rect
	scissorRect.left = 0;
	scissorRect.top = 0;
	scissorRect.right = k_width;
	scissorRect.bottom = k_height; viewport.TopLeftX = 0;


	return true;
}

void Update() // Update logic
{
	// update app logic, such as moving the camera or figuring out what objects are in view
	static float rIncrement = 0.00002f;
	static float gIncrement = 0.00006f;
	static float bIncrement = 0.00009f;

	cbTestColorMultiplierData.testColorMultiplier.x += rIncrement;
	cbTestColorMultiplierData.testColorMultiplier.y += gIncrement;
	cbTestColorMultiplierData.testColorMultiplier.z += bIncrement;

	if (cbTestColorMultiplierData.testColorMultiplier.x >= 1.0 || cbTestColorMultiplierData.testColorMultiplier.x <= 0.0)
	{
		cbTestColorMultiplierData.testColorMultiplier.x = cbTestColorMultiplierData.testColorMultiplier.x >= 1.0 ? 1.0 : 0.0;
		rIncrement = -rIncrement;
	}
	if (cbTestColorMultiplierData.testColorMultiplier.y >= 1.0 || cbTestColorMultiplierData.testColorMultiplier.y <= 0.0)
	{
		cbTestColorMultiplierData.testColorMultiplier.y = cbTestColorMultiplierData.testColorMultiplier.y >= 1.0 ? 1.0 : 0.0;
		gIncrement = -gIncrement;
	}
	if (cbTestColorMultiplierData.testColorMultiplier.z >= 1.0 || cbTestColorMultiplierData.testColorMultiplier.z <= 0.0)
	{
		cbTestColorMultiplierData.testColorMultiplier.z = cbTestColorMultiplierData.testColorMultiplier.z >= 1.0 ? 1.0 : 0.0;
		bIncrement = -bIncrement;
	}

	// copy our ConstantBuffer instance to the mapped constant buffer resource
	memcpy(cbTestColorMultiplierGPUAddress[frameIndex], &cbTestColorMultiplierData, sizeof(cbTestColorMultiplierData));
}

void WaitForPreviousFrame() // Wait until GPU is finished with command list
{
	HRESULT hr;

	// Swap the current RTV buffer and draw to the correct buffer
	frameIndex = swapChain->GetCurrentBackBufferIndex();

	// If the fence value is less than fenceValue, GPU is not finished execution

	if (fence[frameIndex]->GetCompletedValue() < fenceValue[frameIndex])
	{
		hr = fence[frameIndex]->SetEventOnCompletion(fenceValue[frameIndex], fenceEvent);
		if (FAILED(hr))
		{
			g_isRunning = false;
		}

		WaitForSingleObject(fenceEvent, INFINITE);
	}

	fenceValue[frameIndex]++;
}


void UpdatePipeline() // Update D3D (command lists)
{
	HRESULT hr;

	// Wait for previous command allocator to finish
	WaitForPreviousFrame();

	hr = commandAllocator[frameIndex]->Reset();
	if (FAILED(hr))
	{
		g_isRunning = false;
	}

	// Reset command list. Resetting puts list into a recording state. Here initial pipeline state object is passed
	hr = commandList->Reset(commandAllocator[frameIndex], pipelineStateObject);
	if (FAILED(hr))
	{
		g_isRunning = false;
	}


	// Put commands into command list

	// Transition from present state -> RT state to render there
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Handle to current RTV
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// Set Render Target for OM
	commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Clear the RT
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	// Clear DS
	commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	// Draw triangle
	commandList->SetGraphicsRootSignature(rootSignature);

	ID3D12DescriptorHeap* descriptorHeaps[] = { mainDescriptorHeap[frameIndex] };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootDescriptorTable(0, mainDescriptorHeap[frameIndex]->GetGPUDescriptorHandleForHeapStart());

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->IASetIndexBuffer(&indexBufferView);
	commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
	commandList->DrawIndexedInstanced(6, 1, 0, 4, 0);

	// Transition from RT -> present state
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	hr = commandList->Close();
	if (FAILED(hr))
	{
		g_isRunning = false;
	}
}

void Render() // Execute command lists
{
	HRESULT hr;

	UpdatePipeline();

	// Execute array of command lists
	ID3D12CommandList* ppCommandLists[] = { commandList };

	// Execute the array of command lists
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// This signal goes at the end of command queue
	hr = commandQueue->Signal(fence[frameIndex], fenceValue[frameIndex]);
	if (FAILED(hr))
	{
		g_isRunning = false;
	}

	// Present the current backbuffer
	hr = swapChain->Present(0, 0);
	if (FAILED(hr))
	{
		g_isRunning = false;
	}
}

void Cleanup() // release COM objects and clean up memory
{
	for (int i = 0; i < frameBufferCount; i++)
	{
		frameIndex = i;
		WaitForPreviousFrame();
	}

	BOOL fs = false;
	if (swapChain->GetFullscreenState(&fs, NULL))
	{
		swapChain->SetFullscreenState(FALSE, NULL);
	}

	SAFE_RELEASE(device);
	SAFE_RELEASE(swapChain);
	SAFE_RELEASE(commandQueue);
	SAFE_RELEASE(rtvDescriptorHeap);
	SAFE_RELEASE(commandList);

	for (int i = 0; i < frameBufferCount; i++)
	{
		SAFE_RELEASE(renderTargets[i]);
		SAFE_RELEASE(commandAllocator[i]);
		SAFE_RELEASE(fence[i]);
		SAFE_RELEASE(mainDescriptorHeap[i]);
		SAFE_RELEASE(constantBufferUploadHeap[i]);
	}

	SAFE_RELEASE(pipelineStateObject);
	SAFE_RELEASE(rootSignature);
	SAFE_RELEASE(vertexBuffer);
	SAFE_RELEASE(indexBuffer);
	SAFE_RELEASE(depthStencilBuffer);
	SAFE_RELEASE(dsvDescriptorHeap);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool InitializeWindow(HINSTANCE hInstance, int showWnd, int width, int height, bool fullscreen)
{
	if (fullscreen)
	{
		HMONITOR hMon = MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hMon, &mi);

		width = mi.rcMonitor.right - mi.rcMonitor.left;
		height = mi.rcMonitor.bottom - mi.rcMonitor.top;
	}

	WNDCLASSEX wc;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 2);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = k_windowName;
	wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(0, L"Error registering WNDCLASSEX!", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	g_hwnd = CreateWindowEx(0, k_windowName, k_windowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, k_width, k_height, nullptr, NULL, hInstance, nullptr);

	if (!g_hwnd)
	{
		MessageBox(NULL, L"Error creating window!", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	if (fullscreen)
	{
		SetWindowLong(g_hwnd, GWL_STYLE, 0);
	}

	ShowWindow(g_hwnd, showWnd);
	UpdateWindow(g_hwnd);

	return true;
}

void MainLoop()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	while (g_isRunning)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				break;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Update Game Logic
			Update();
			Render();
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			if (MessageBox(0, L"Are you sure you want to exit?", L"Really?", MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				DestroyWindow(hWnd);
			}
		}
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd)
{
	if (!InitializeWindow(hInstance, nShowCmd, k_width, k_height, k_fullScreen))
	{
		MessageBox(0, L"Window initialization failed!", L"Error", MB_OK);
		return 1;
	}

	if (!InitD3D())
	{
		MessageBox(0, L"Failed to initialize DirectX 12", L"Error", MB_OK);
		Cleanup();
		return 1;
	}

	MainLoop();

	WaitForPreviousFrame();

	CloseHandle(fenceEvent);

	Cleanup();

	return 0;
}