#pragma once

#include <d3d12.h>
#include <d3dx12.h>
#include <tuple>
#include <Render/RenderTarget.h>

namespace alexis
{
	namespace utils
	{
		inline DXGI_FORMAT GetFormatForSrv(DXGI_FORMAT inFormat)
		{
			switch (inFormat)
			{
			case DXGI_FORMAT_R24G8_TYPELESS:
			{
				return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
			}
			default:
				return inFormat;
			}
		}

		inline DXGI_FORMAT GetFormatForDsv(DXGI_FORMAT inFormat)
		{
			switch (inFormat)
			{
			case DXGI_FORMAT_R24G8_TYPELESS:
			{
				return DXGI_FORMAT_D24_UNORM_S8_UINT;
			}
			default:
				return inFormat;
			}
		}

		inline std::tuple<std::wstring, RenderTarget::Slot, bool> ParseRTName(const std::wstring& texturePath)
		{
			if (texturePath[0] == L'$')
			{
				auto indexPos = texturePath.find(L'#');
				auto rtName = texturePath.substr(1, indexPos - 1);

				int index = 0;

				if (indexPos != std::wstring::npos)
				{
					auto indexStr = texturePath.substr(indexPos + 1);
					if (indexStr == L"Depth")
					{
						index = RenderTarget::Slot::DepthStencil;
					}
					else
					{
						index = _wtoi(indexStr.c_str());
					}
				}

				return { rtName, static_cast<RenderTarget::Slot>(index), true };
			}
			else
			{
				return { texturePath, RenderTarget::Slot::NumAttachmentPoints, false };
			}

		}
	}
}

