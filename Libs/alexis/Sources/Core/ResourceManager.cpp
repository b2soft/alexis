#include <Precompiled.h>

#include "ResourceManager.h"

#include <Core/Render.h>
#include <Core/CommandManager.h>
#include <Render/CommandContext.h>

namespace alexis
{

	ResourceManager::ResourceManager()
	{
		auto commandManager = Render::GetInstance()->GetCommandManager();
		m_copyContext = commandManager->CreateCommandContext(D3D12_COMMAND_LIST_TYPE_COPY);
	}

	TextureBuffer* ResourceManager::GetTexture(const std::wstring& path)
	{
		auto it = m_textures.find(path);
		if (it != m_textures.end())
		{
			return &(it->second);
		}

		it = LoadTexture(path);
		return &(it->second);
	}

	ResourceManager::TextureMap::iterator ResourceManager::LoadTexture(const std::wstring& path)
	{
		fs::path filePath(path);

		if (!fs::exists(filePath))
		{
			throw std::exception("File not found!");
		}

		TexMetadata metadata;
		ScratchImage scratchImage;

		if (filePath.extension() == ".dds")
		{
			ThrowIfFailed(
				LoadFromDDSFile(path.c_str(), DDS_FLAGS_FORCE_RGB, &metadata, scratchImage)
			);
		}
		else if (filePath.extension() == ".hdr")
		{
			ThrowIfFailed(
				LoadFromHDRFile(path.c_str(), &metadata, scratchImage)
			);
		}
		else if (filePath.extension() == ".tga")
		{
			ThrowIfFailed(
				LoadFromTGAFile(path.c_str(), &metadata, scratchImage)
			);
		}
		else
		{
			LoadFromWICFile(path.c_str(), WIC_FLAGS_FORCE_RGB, &metadata, scratchImage);
		}

		D3D12_RESOURCE_DESC textureDesc = {};
		switch (metadata.dimension)
		{
		case TEX_DIMENSION_TEXTURE1D:
			textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT16>(metadata.arraySize));
			break;
		case TEX_DIMENSION_TEXTURE2D:
			textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>(metadata.height), static_cast<UINT16>(metadata.arraySize));
			break;
		case TEX_DIMENSION_TEXTURE3D:
			textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>(metadata.height), static_cast<UINT16>(metadata.depth));
			break;
		default:
			throw std::exception("Invalid texture dimension!");
			break;
		}

		std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
		const Image* images = scratchImage.GetImages();
		for (int i = 0; i < scratchImage.GetImageCount(); ++i)
		{
			auto& subresource = subresources[i];
			subresource.RowPitch = images[i].rowPitch;
			subresource.SlicePitch = images[i].slicePitch;
			subresource.pData = images[i].pixels;
		}

		TextureBuffer texture;
		texture.Create(metadata.width, metadata.height, metadata.format, metadata.mipLevels);

		m_copyContext->InitializeTexture(texture, subresources.size(), subresources.data());
		m_copyContext->Flush(true);

		return m_textures.emplace(path, std::move(texture)).first;
	}

}