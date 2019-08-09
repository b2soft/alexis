#include "Precompiled.h"

#include <Shlwapi.h>
#include <dxgidebug.h>

#include "Application.h"
#include "b2Game.h"

void ReportLiveObjects()
{
	IDXGIDebug1* dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));

	//dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_IGNORE_INTERNAL);
	//dxgiDebug->Release();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShowCmd)
{
	int retCode = 0;

	// Set the working directory to the path of the executable
	WCHAR path[MAX_PATH];
	HMODULE hModule = GetModuleHandle(NULL);
	if (GetModuleFileName(hModule, path, MAX_PATH) > 0)
	{
		PathRemoveFileSpecW(path);
		SetCurrentDirectoryW(path);
	}

	Application::Create(hInstance);
	{
		std::shared_ptr<b2Game> game = std::make_shared<b2Game>(L"b2Game Sample", 1280, 720);
		retCode = Application::Get().Run(game);
	}

	Application::Destroy();

	atexit(&ReportLiveObjects);

	return retCode;
}