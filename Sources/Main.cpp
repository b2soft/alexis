#include "Precompiled.h"

#include <Shlwapi.h>

#include "SampleApp.h"

#include <Core/Core.h>
#include <Render/Render.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
	// Set the working directory to the path of the executable
	WCHAR path[MAX_PATH];
	HMODULE hModule = GetModuleHandle(NULL);
	if (GetModuleFileName(hModule, path, MAX_PATH) > 0)
	{
		PathRemoveFileSpecW(path);
		SetCurrentDirectoryW(path);
	}

#if defined(_DEBUG) && defined(REPORT_LIVE_OBJECTS)
	atexit(&ReportLiveObjects);
#endif

	int returnCode = 0;
	alexis::Core::Create(hInstance);
	{
		SampleApp sample;
		returnCode = alexis::Core::Get().Run(&sample);
	}
	alexis::Core::Destroy();

	return returnCode;
}