#pragma once

#include <unordered_map>

#include <Render/RenderTarget.h>

namespace alexis
{
	class RenderTargetManager
	{
	public:
		void EmplaceTarget(const std::wstring& name, std::unique_ptr<RenderTarget> target);

		void Resize(uint32_t width, uint32_t height);

		RenderTarget* GetRenderTarget(const std::wstring& name);

		D3D12_RT_FORMAT_ARRAY GetRTFormats(const std::wstring& name) const;

		DXGI_FORMAT GetDSFormat(const std::wstring& name) const;

		void AddManagedSRV(const std::wstring& rtName, D3D12_CPU_DESCRIPTOR_HANDLE handle);
		void RebindManagedSRV();

	private:
		std::unordered_map<std::wstring, std::unique_ptr<RenderTarget>> m_managedTargets;

		std::vector<std::pair<std::wstring, CD3DX12_CPU_DESCRIPTOR_HANDLE>> m_managedHandles;
	};
}