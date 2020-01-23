#include <Precompiled.h>

#include "RenderTarget.h"


namespace alexis
{

	RenderTarget::RenderTarget() :
		m_textures(Slot::NumAttachmentPoints)
	{

	}

	void RenderTarget::AttachTexture(const TextureBuffer& texture, Slot slot)
	{
		auto texResource = texture.GetResource();

		m_textures[slot].SetFromResource(texResource);

		if (texResource)
		{
			auto desc = texResource->GetDesc();
			m_size.x = static_cast<uint32_t>(desc.Width);
			m_size.y = static_cast<uint32_t>(desc.Height);
		}
	}

	const alexis::TextureBuffer& RenderTarget::GetTexture(Slot slot) const
	{
		return m_textures[slot];
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

	D3D12_VIEWPORT RenderTarget::GetViewport(DirectX::XMFLOAT2 scale /*= { 1.0f, 1.0f }*/, DirectX::XMFLOAT2 bias /*= { 0.0f, 0.0f }*/, float minDepth /*= 0.0f*/, float maxDepth /*= 1.0f*/) const
	{
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

		D3D12_VIEWPORT viewport =
		{
			(width * bias.x),
			(height * bias.y),
			(width * scale.x),
			(height * scale.y),
			minDepth,
			maxDepth
		};

		return viewport;
	}

	const std::vector<alexis::TextureBuffer>& RenderTarget::GetTextures() const
	{
		return m_textures;
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


}
