#pragma once

#include "Resource.h"

#include "DescriptorAllocation.h"

#include "../d3dx12.h"

#include <mutex>
#include <unordered_map>

enum class TextureUsage
{
	Albedo,
	Diffuse = Albedo,       // Treat Diffuse and Albedo textures the same.
	Heightmap,
	Depth = Heightmap,      // Treat height and depth textures the same.
	Normalmap,
	RenderTarget,           // Texture is used as a render target.
};

class Texture : public Resource
{
public:
	explicit Texture(TextureUsage textureUsage = TextureUsage::Albedo, const std::wstring& name = L"");
	explicit Texture(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue = nullptr, TextureUsage textureUsage = TextureUsage::Albedo, const std::wstring& name = L"");
	explicit Texture(Microsoft::WRL::ComPtr<ID3D12Resource> resource, TextureUsage textureUsage = TextureUsage::Albedo, const std::wstring& name = L"");

	Texture(const Texture& copy);
	Texture& operator=(const Texture& copy);

	Texture(Texture&& other);
	Texture& operator=(Texture&& copy);

	virtual ~Texture();

	TextureUsage GetTextureUsage() const
	{
		return m_textureUsage;
	}

	void SetTextureUsage(TextureUsage usage)
	{
		m_textureUsage = usage;
	}

	void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1);

	virtual void CreateViews();

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const;

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const;

	static bool CheckSRVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
	{
		return ((formatSupport & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE) != 0 ||
			(formatSupport & D3D12_FORMAT_SUPPORT1_SHADER_LOAD) != 0);
	}

	static bool CheckUAVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
	{
		return ((formatSupport & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) != 0);
	}

	static bool CheckRTVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
	{
		return ((formatSupport & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0);
	}

	static bool CheckDSVSupport(D3D12_FORMAT_SUPPORT1 formatSupport)
	{
		return ((formatSupport & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0);
	}

	static bool IsUAVCompatibleFormat(DXGI_FORMAT format);
	static bool IsDepthFormat(DXGI_FORMAT format);

	static DXGI_FORMAT GetTypelessFormat(DXGI_FORMAT format);

private:
	DescriptorAllocation CreateSRV(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const;
	DescriptorAllocation CreateUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const;

	mutable std::unordered_map<size_t, DescriptorAllocation> m_srvs;
	mutable std::unordered_map<size_t, DescriptorAllocation> m_uavs;

	mutable std::mutex m_srvMutex;
	mutable std::mutex m_uavMutex;

	DescriptorAllocation m_rtv;
	DescriptorAllocation m_dsv;

	TextureUsage m_textureUsage;

};