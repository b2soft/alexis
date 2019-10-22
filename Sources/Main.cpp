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
	alexis::Core::Create(hInstance);
	{
		SampleApp sample;
		returnCode = alexis::Core::Get().Run(&sample);
	}
	alexis::Core::Destroy();

	return returnCode;

	//SampleApp sample(1280, 720, L"Alexis Sample App");
	//return alexis::Win32Application::Run(&sample, hInstance, nCmdShow);
}