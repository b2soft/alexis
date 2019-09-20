#pragma once

#include <cstdint>
#include <vector>

#include "Texture.h"

enum AttachmentPoint
{
	Color0,
	Color1,
	DepthStencil,
	NumAttachmentPoints
};

class RenderTarget
{
public:
	// Empty RT
	RenderTarget();

	RenderTarget(const RenderTarget&) = default;
	RenderTarget& operator=(const RenderTarget&) = default;

	RenderTarget(RenderTarget&&) = default;
	RenderTarget& operator=(RenderTarget&&) = default;

	// Attach a texture to RT
	// Texture will be copied to the texture array
	void AttachTexture(AttachmentPoint attachmentPoint, const Texture& texture);
	const Texture& GetTexture(AttachmentPoint attachmentPoint) const;

	// Resize all textures associated
	void Resize(uint32_t width, uint32_t height);

	// Get a list of textures attached to RT
	const std::vector<Texture>& GetTextures() const;

	// Get RT formats. Needed for PSO
	D3D12_RT_FORMAT_ARRAY GetRenderTargetFormats() const;

	// Get the DS format
	DXGI_FORMAT GetDepthStencilFormat() const;

private:
	std::vector<Texture> m_textures;
};