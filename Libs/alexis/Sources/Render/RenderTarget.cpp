#include <Precompiled.h>

#include "RenderTarget.h"
#include <Render/Render.h>

namespace alexis
{

	RenderTarget::RenderTarget(bool isFullscreen) :
		m_textures(Slot::NumAttachmentPoints),
		m_isFullscreen(isFullscreen)
	{
		m_rtvs.resize(Slot::NumAttachmentPoints, {});
	}

	void RenderTarget::AttachTexture(const TextureBuffer& texture, Slot slot)
	{
		AttachTexture(texture.GetResource(), slot);
	}

	void RenderTarget::AttachTexture(ID3D12Resource* texture, Slot slot)
	{
		auto texDesc = texture->GetDesc();

		auto* render = Render::GetInstance();

		m_textures[slot].SetFromResource(texture);

		if (slot == Slot::DepthStencil)
		{
			if (m_dsv.ptr == 0)
			{
				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

				if (texDesc.Format == DXGI_FORMAT_R24G8_TYPELESS)
				{
					dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				}

				auto dsvHandle = render->AllocateDSV(texture, dsvDesc);

				m_dsv = dsvHandle.CpuPtr;
			}
		}
		else
		{
			if (m_rtvs[slot].ptr == 0)
			{
				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

				rtvDesc.Format = texDesc.Format;
				auto rtvHandle = render->AllocateRTV(texture, rtvDesc);

				m_rtvs[slot] = rtvHandle.CpuPtr;
			}
		}

		if (texture)
		{
			auto desc = texture->GetDesc();
			m_size.x = static_cast<uint32_t>(desc.Width);
			m_size.y = static_cast<uint32_t>(desc.Height);
		}
	}

	const alexis::TextureBuffer& RenderTarget::GetTexture(Slot slot) const
	{
		return m_textures[slot];
	}

	alexis::TextureBuffer& RenderTarget::GetTexture(Slot slot)
	{
		return m_textures[slot];
	}

	void RenderTarget::Reset()
	{
		for (auto& texture : m_textures)
		{
			if (texture.IsValid())
			{
				texture.Reset();
			}
		}
	}

	void RenderTarget::Resize(uint32_t width, uint32_t height)
	{
		Resize(XMUINT2{ width, height });
	}

	void RenderTarget::Resize(DirectX::XMUINT2 size)
	{
		m_size = size;
		for (auto& texture : m_textures)
		{
			texture.Resize(m_size.x, m_size.y);
		}
	}

	DirectX::XMUINT2 RenderTarget::GetSize() const
	{
		return m_size;
	}

	Viewport RenderTarget::GetViewport(DirectX::XMFLOAT2 scale /*= { 1.0f, 1.0f }*/, DirectX::XMFLOAT2 bias /*= { 0.0f, 0.0f }*/, float minDepth /*= 0.0f*/, float maxDepth /*= 1.0f*/) const
	{
		Viewport viewport;

		UINT64 width = 0;
		UINT height = 0;

		for (const auto& texture : m_textures)
		{
			if (texture.IsValid())
			{
				auto desc = texture.GetResourceDesc();
				width = std::max(width, desc.Width);
				height = std::max(height, desc.Height);
			}
		}

		viewport.Viewport =
		{
			(width * bias.x),
			(height * bias.y),
			(width * scale.x),
			(height * scale.y),
			minDepth,
			maxDepth
		};

		viewport.ScissorRect = CD3DX12_RECT(0, 0, width, height);

		return viewport;
	}

	const std::vector<alexis::TextureBuffer>& RenderTarget::GetTextures() const
	{
		return m_textures;
	}

	const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& RenderTarget::GetRtvs() const
	{
		return m_rtvs;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE RenderTarget::GetRtv(Slot slot) const
	{
		return m_rtvs[slot];
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE RenderTarget::GetDsv() const
	{
		return m_dsv;
	}

	D3D12_RT_FORMAT_ARRAY RenderTarget::GetFormat() const
	{
		D3D12_RT_FORMAT_ARRAY rtvFormats = {};

		for (int i = Slot::Slot0; i < Slot::DepthStencil; ++i)
		{
			const TextureBuffer& texture = m_textures[i];
			if (texture.IsValid())
			{
				rtvFormats.RTFormats[rtvFormats.NumRenderTargets++] = texture.GetResourceDesc().Format;
			}
		}

		return rtvFormats;
	}

	DXGI_FORMAT RenderTarget::GetDSFormat() const
	{
		DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
		const TextureBuffer& depthStencilTexture = m_textures[Slot::DepthStencil];
		if (depthStencilTexture.IsValid())
		{
			dsvFormat = depthStencilTexture.GetResourceDesc().Format;
		}

		return dsvFormat;
	}


	bool RenderTarget::IsFullscreen() const
	{
		return m_isFullscreen;
	}

}
