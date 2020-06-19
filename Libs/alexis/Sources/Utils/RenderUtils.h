#pragma once

#include <d3d12.h>
#include <d3dx12.h>

namespace alexis
{
	namespace utils
	{
		DXGI_FORMAT GetFormatForSrv(DXGI_FORMAT inFormat)
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
	}
}

