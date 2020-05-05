#pragma once

#include <unordered_map>

#include <Render/RenderTarget.h>

namespace alexis
{
	class RenderTargetManager
	{
	public:
		void EmplaceTarget(const std::wstring& name, std::unique_ptr<RenderTarget> target)
		{
			m_managedTargets.emplace(name, std::move(target));
		}

		void Resize(uint32_t width, uint32_t height)
		{
			for (auto& [_, rt] : m_managedTargets)
			{
				if (rt->IsFullscreen())
				{
					rt->Resize(width, height);
				}
			}
		}

		RenderTarget* GetRenderTarget(const std::wstring& name)
		{
			assert(m_managedTargets.find(name) != m_managedTargets.end());

			return m_managedTargets.at(name).get();
		}

		D3D12_RT_FORMAT_ARRAY GetRTFormats(const std::wstring& name) const
		{
			assert(m_managedTargets.find(name) != m_managedTargets.end());

			return m_managedTargets.at(name)->GetFormat();
		}

		DXGI_FORMAT GetDSFormat(const std::wstring& name) const
		{
			assert(m_managedTargets.find(name) != m_managedTargets.end());

			return m_managedTargets.at(name)->GetDSFormat();
		}

	private:
		std::unordered_map<std::wstring, std::unique_ptr<RenderTarget>> m_managedTargets;
	};
}