#pragma once

#include <vector>
#include <DirectXMath.h>

#include <Render/Buffers/GpuBuffer.h>
#include <Render/Viewport.h>

namespace alexis
{
	class RenderTarget
	{
	public:
		enum Slot
		{
			Slot0 = 0,
			Slot1,
			Slot2,
			Slot3,
			DepthStencil,

			NumAttachmentPoints
		};

		RenderTarget(bool isFullscreen = false);

		RenderTarget(const RenderTarget&) = default;
		RenderTarget& operator=(const RenderTarget&) = default;

		RenderTarget(RenderTarget&&) = default;
		RenderTarget& operator=(RenderTarget&&) = default;

		void AttachTexture(const TextureBuffer& texture, Slot slot);
		void AttachTexture(ID3D12Resource* texture, Slot slot);
		const TextureBuffer& GetTexture(Slot slot) const;
		TextureBuffer& GetTexture(Slot slot);
		void Reset();

		void Resize(uint32_t width, uint32_t height);
		void Resize(DirectX::XMUINT2 size);

		DirectX::XMUINT2 GetSize() const;

		Viewport GetViewport(DirectX::XMFLOAT2 scale = { 1.0f, 1.0f }, DirectX::XMFLOAT2 bias = { 0.0f, 0.0f }, float minDepth = 0.0f, float maxDepth = 1.0f) const;
		const std::vector<TextureBuffer>& GetTextures() const;

		const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& GetRtvs() const;
		const D3D12_CPU_DESCRIPTOR_HANDLE GetRtv(Slot slot) const;
		const D3D12_CPU_DESCRIPTOR_HANDLE GetDsv() const;

		D3D12_RT_FORMAT_ARRAY GetFormat() const;
		DXGI_FORMAT GetDSFormat() const;

		bool IsFullscreen() const;

	private:
		std::vector<TextureBuffer> m_textures;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_rtvs;
		D3D12_CPU_DESCRIPTOR_HANDLE m_dsv{};
		DirectX::XMUINT2 m_size{ 0,0 };
		bool m_isFullscreen{ false }; // will be resized with window size
	};

}