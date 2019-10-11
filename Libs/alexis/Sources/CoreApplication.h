#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <string>

#include "Win32Application.h"

namespace alexis
{
	class CoreApplication
	{
	public:
		CoreApplication(UINT width, UINT height, std::wstring name);
		virtual ~CoreApplication() = default;

		virtual void OnInit() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnRender() = 0;
		virtual void OnDestroy() = 0;

		virtual void OnKeyDown(UINT8) {}
		virtual void OnKeyUp(UINT8) {}

		UINT GetWidth() const { return m_width; }
		UINT GetHeight() const { return m_height; }
		const WCHAR* GetTitle() const { return m_windowTitle.c_str(); }

		void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

	protected:
		std::wstring GetAssetFullPath(LPCWSTR assetName);
		Microsoft::WRL::ComPtr<IDXGIAdapter4> GetHardwareAdapter(_In_ IDXGIFactory2* factory);
		void SetCustomWindowText(LPCWSTR text);

		UINT m_width;
		UINT m_height;
		float m_aspectRatio;

		bool m_useWarpDevice;

	private:
		std::wstring m_assetsPath;

		std::wstring m_windowTitle;
	};
}

