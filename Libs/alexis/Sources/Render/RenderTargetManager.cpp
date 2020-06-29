#include <Precompiled.h>

#include "RenderTargetManager.h"

#include <Render/Render.h>
#include <Utils/RenderUtils.h>


namespace alexis
{

	void RenderTargetManager::EmplaceTarget(const std::wstring& name, std::unique_ptr<RenderTarget> target)
	{
		m_managedTargets.emplace(name, std::move(target));
	}

	void RenderTargetManager::Resize(uint32_t width, uint32_t height)
	{
		for (auto& [_, rt] : m_managedTargets)
		{
			if (rt->IsFullscreen())
			{
				rt->Resize(width, height);
			}
		}

		RebindManagedSRV();
	}

	alexis::RenderTarget* RenderTargetManager::GetRenderTarget(const std::wstring& name)
	{
		assert(m_managedTargets.find(name) != m_managedTargets.end());

		return m_managedTargets.at(name).get();
	}

	D3D12_RT_FORMAT_ARRAY RenderTargetManager::GetRTFormats(const std::wstring& name) const
	{
		assert(m_managedTargets.find(name) != m_managedTargets.end());

		return m_managedTargets.at(name)->GetFormat();
	}

	DXGI_FORMAT RenderTargetManager::GetDSFormat(const std::wstring& name) const
	{
		assert(m_managedTargets.find(name) != m_managedTargets.end());

		return m_managedTargets.at(name)->GetDSFormat();
	}

	void RenderTargetManager::AddManagedSRV(const std::wstring& rtName, D3D12_CPU_DESCRIPTOR_HANDLE handle)
	{
		m_managedHandles.emplace_back(rtName, handle);
	}

	void RenderTargetManager::RebindManagedSRV()
	{
		for (auto& [name, handle] : m_managedHandles)
		{
			auto* render = alexis::Render::GetInstance();
			auto [texName, slot, isRT] = utils::ParseRTName(name);
			const auto& texture = m_managedTargets[texName]->GetTexture(slot);
			const auto resDesc = texture.GetResourceDesc();

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = utils::GetFormatForSrv(resDesc.Format);
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = resDesc.MipLevels;

			// TODO: Found better way to distinct cubemaps?
			if (name.find(L"CUBEMAP") != std::wstring::npos)
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MostDetailedMip = 0;
				srvDesc.TextureCube.MipLevels = resDesc.MipLevels;
				srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
			}

			render->UpdateSRV(texture.GetResource(), srvDesc, handle);
		}
	}

}

