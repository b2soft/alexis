#pragma once

#include <vector>
#include <DirectXMath.h>

#include <Render/Buffers/GpuBuffer.h>

namespace alexis
{
	class RenderTarget
	{
		enum Slot
		{
			Slot0 = 0,
			Slot1,
			Slot2,
			Slot3,
			DepthStencil,

			NumAttachmentPoints
		};

	public:
		RenderTarget();

		RenderTarget(const RenderTarget&) = default;
		RenderTarget& operator=(const RenderTarget&) = default;

		RenderTarget(RenderTarget&&) = default;
		RenderTarget& operator=(RenderTarget&&) = default;

		void AttachTexture(const TextureBuffer& texture, Slot slot);
		const TextureBuffer& GetTexture(Slot slot) const;

		//TODO: Resize impl
		void Resize(uint32_t width, uint32_t height);
		void Resize(DirectX::XMUINT2 size);

		DirectX::XMUINT2 GetSize() const;

		D3D12_VIEWPORT GetViewport(DirectX::XMFLOAT2 scale = { 1.0f, 1.0f }, DirectX::XMFLOAT2 bias = { 0.0f, 0.0f }, float minDepth = 0.0f, float maxDepth = 1.0f) const;
		const std::vector<TextureBuffer>& GetTextures() const;

		D3D12_RT_FORMAT_ARRAY GetFormat() const;
		DXGI_FORMAT GetDSFormat() const;

	private:
		std::vector<TextureBuffer> m_textures;
		DirectX::XMUINT2 m_size{ 0,0 };
	};

}