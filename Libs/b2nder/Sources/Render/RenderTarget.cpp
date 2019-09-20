#include <Precompiled.h>

#include "RenderTarget.h"


RenderTarget::RenderTarget()
	: m_textures(AttachmentPoint::NumAttachmentPoints)
{
}

void RenderTarget::AttachTexture(AttachmentPoint attachmentPoint, const Texture& texture)
{
	m_textures[attachmentPoint] = texture;
}

const Texture& RenderTarget::GetTexture(AttachmentPoint attachmentPoint) const
{
	return m_textures[attachmentPoint];
}

void RenderTarget::Resize(uint32_t width, uint32_t height)
{
	for (auto& texture : m_textures)
	{
		texture.Resize(width, height);
	}
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

