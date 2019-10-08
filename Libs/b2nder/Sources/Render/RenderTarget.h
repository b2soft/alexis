#pragma once

#include <cstdint>
#include <vector>

#include <DirectXMath.h> 

#include "Texture.h"

enum AttachmentPoint
{
	Color0,
	Color1,
	Color2,
	Color3,
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
	void Resize(DirectX::XMUINT2 size);

	DirectX::XMUINT2 GetSize() const;

	D3D12_VIEWPORT GetViewport(DirectX::XMFLOAT2 scale = { 1.0f, 1.0f }, DirectX::XMFLOAT2 bias = { 0.0f, 0.0f }, float minDepth = 0.0f, float maxDepth = 1.0f) const;

	// Get a list of textures attached to RT
	const std::vector<Texture>& GetTextures() const;

	// Get RT formats. Needed for PSO
	D3D12_RT_FORMAT_ARRAY GetRenderTargetFormats() const;

	// Get the DS format
	DXGI_FORMAT GetDepthStencilFormat() const;

private:
	std::vector<Texture> m_textures;
	DirectX::XMUINT2 m_size{ 0, 0 };
};