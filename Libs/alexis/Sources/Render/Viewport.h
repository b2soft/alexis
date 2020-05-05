#pragma once

#include <d3d12.h>
#include <d3dx12.h>

namespace alexis
{
	struct Viewport
	{
		D3D12_VIEWPORT Viewport;
		CD3DX12_RECT ScissorRect;
	};
}