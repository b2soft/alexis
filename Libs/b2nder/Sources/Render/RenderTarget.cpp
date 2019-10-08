#include <Precompiled.h>

#include "RenderTarget.h"


RenderTarget::RenderTarget()
	: m_textures(AttachmentPoint::NumAttachmentPoints)
{
}

void RenderTarget::AttachTexture(AttachmentPoint attachmentPoint, const Texture& texture)
{
	m_textures[attachmentPoint] = texture;

	auto texResource = texture.GetD3D12Resource();
	if (texResource)
	{
		auto desc = texResource->GetDesc();
		m_size.x = static_cast<uint32_t>(desc.Width);
		m_size.y = static_cast<uint32_t>(desc.Height);
	}
}

const Texture& RenderTarget::GetTexture(AttachmentPoint attachmentPoint) const
{
	return m_textures[attachmentPoint];
}

void RenderTarget::Resize(DirectX::XMUINT2 size)
{
	m_size = size;
	for (auto& texture : m_textures)
	{
		texture.Resize(m_size.x, m_size.y);
	}
}

void RenderTarget::Resize(uint32_t width, uint32_t height)
{
	Resize(XMUINT2{ width, height });
}

DirectX::XMUINT2 RenderTarget::GetSize() const
{
	return m_size;
}

D3D12_VIEWPORT RenderTarget::GetViewport(DirectX::XMFLOAT2 scale /*= { 1.0f, 1.0f }*/, DirectX::XMFLOAT2 bias /*= { 0.0f, 0.0f }*/, float minDepth /*= 0.0f*/, float maxDepth /*= 1.0f*/) const
{
	UINT64 width = 0;
	UINT height = 0;

	for (int i = AttachmentPoint::Color0; i <= AttachmentPoint::DepthStencil - 1; ++i)
	{
		const Texture& texture = m_textures[i];

		if (texture.IsValid())
		{
			auto desc = texture.GetD3D12ResourceDesc();
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

const std::vector<Texture>& RenderTarget::GetTextures() const
{
	return m_textures;
}

D3D12_RT_FORMAT_ARRAY RenderTarget::GetRenderTargetFormats() const
{
	D3D12_RT_FORMAT_ARRAY rtvFormats = {};

	for (int i = AttachmentPoint::Color0; i <= AttachmentPoint::DepthStencil - 1; ++i)
	{
		const Texture& texture = m_textures[i];
		if (texture.IsValid())
		{
			rtvFormats.RTFormats[rtvFormats.NumRenderTargets++] = texture.GetD3D12ResourceDesc().Format;
		}
	}

	return rtvFormats;
}

DXGI_FORMAT RenderTarget::GetDepthStencilFormat() const
{
	DXGI_FORMAT dsvFormat = DXGI_FORMAT_UNKNOWN;
	const Texture& depthStencilTexture = m_textures[AttachmentPoint::DepthStencil];
	if (depthStencilTexture.IsValid())
	{
		dsvFormat = depthStencilTexture.GetD3D12ResourceDesc().Format;
	}

	return dsvFormat;
}

