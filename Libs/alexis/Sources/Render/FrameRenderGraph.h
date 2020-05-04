#pragma once

#include <d3dx12.h>

namespace alexis
{
	class FrameRenderGraph
	{
	public:
		~FrameRenderGraph();
		void Render();

	private:
		CD3DX12_RECT m_scissorRect{ 0, 0, LONG_MAX, LONG_MAX };
	};
}