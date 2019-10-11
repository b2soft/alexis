#include <Precompiled.h>

#include "CoreApplication.h"
#include "CoreHelpers.h"

using namespace Microsoft::WRL;

namespace alexis
{
	CoreApplication::CoreApplication(UINT width, UINT height, std::wstring name) :
		m_width(width),
		m_height(height),
		m_windowTitle(name),
		m_useWarpDevice(false)
	{
		WCHAR assetsPath[512];
		GetAssetsPath(assetsPath, _countof(assetsPath));
		m_assetsPath = assetsPath;

		m_aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	}

	void CoreApplication::ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc)
	{
		for (int i = 1; i < argc; ++i)
		{
			if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
				_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
			{
				m_useWarpDevice = true;
				m_windowTitle = m_windowTitle = L" (WARP)";
			}
		}
	}

	std::wstring CoreApplication::GetAssetFullPath(LPCWSTR assetName)
	{
		return m_assetsPath + assetName;
	}

	_Use_decl_annotations_
		Microsoft::WRL::ComPtr<IDXGIAdapter4> CoreApplication::GetHardwareAdapter(_In_ IDXGIFactory2* factory)
	{
		ComPtr<IDXGIFactory4> dxgiFactory;
		UINT createFactoryFlags = 0;
#if defined(_DEBUG)
		createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

		ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

		ComPtr<IDXGIAdapter1> dxgiAdapter1;
		ComPtr<IDXGIAdapter4> dxgiAdapter4;

		if (m_useWarpDevice)
		{
			ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
			ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
		}
		else
		{
			SIZE_T maxDedicatedVideoMemory = 0;
			for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
			{
				DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
				dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

				// Check to see if the adapter can create a D3D12 device without actually creating it.
				// The adapter with the largest dedicated video memory is favored.
				if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
					SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
					dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
				{
					maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
					ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
				}
			}
		}

		return dxgiAdapter4;
	}

	void CoreApplication::SetCustomWindowText(LPCWSTR text)
	{
		std::wstring windowText = m_windowTitle + L": " + text;
		SetWindowText(Win32Application::GetHwnd(), windowText.c_str());
	}

}