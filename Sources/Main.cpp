#include "Precompiled.h"
//
#include <Shlwapi.h>
//#include <dxgidebug.h>
//
//#include <Application.h>
//#include "b2Game.h"
//
//void ReportLiveObjects()
//{
//#if defined(_DEBUG)
//	IDXGIDebug1* dxgiDebug;
//	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
//
//	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
//	dxgiDebug->Release();
//#endif
//}

#include "SampleApp.h"

#include <Core/Core.h>
#include <Core/Render.h>

class Sample : public IGame
{
public:
	virtual bool LoadContent() override
	{
		return true;
	}


	virtual void UnloadContent() override
	{
		
	}


	virtual void OnUpdate(float dt) override
	{
		
	}


	virtual void OnRender() override
	{
		auto render = Render::GetInstance();
		auto commandList = render->GetGraphicsCommandList();

		auto backBuffer = render->GetCurrentBackBufferResource();
		auto rtvHandle = render->GetCurrentBackBufferRTV();

		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		// Record actual commands
		const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
		commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		//m_commandList->ExecuteBundle(m_bundle.Get());

		// Transit back buffer back to Present state
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	}

};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	// Set the working directory to the path of the executable
	WCHAR path[MAX_PATH];
	HMODULE hModule = GetModuleHandle(NULL);
	if (GetModuleFileName(hModule, path, MAX_PATH) > 0)
	{
		PathRemoveFileSpecW(path);
		SetCurrentDirectoryW(path);
	}

	//atexit(&ReportLiveObjects);

	int returnCode = 0;
	Core::Create(hInstance);
	{
		std::shared_ptr<Sample> sample = std::make_shared<Sample>();
		returnCode = Core::Get().Run(sample);
	}
	Core::Destroy();

	return returnCode;

	//SampleApp sample(1280, 720, L"Alexis Sample App");
	//return alexis::Win32Application::Run(&sample, hInstance, nCmdShow);
}