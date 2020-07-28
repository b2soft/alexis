#include <Shlwapi.h>

#include <Core/Core.h>
#include "EditorApp.h"

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

	int returnCode = 0;
	alexis::Core::Create(hInstance);
	{
		EditorApp editor;
		returnCode = alexis::Core::Get().Run(&editor);
	}
	alexis::Core::Destroy();

	return returnCode;
}