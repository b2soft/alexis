#pragma once

#include <unordered_map>

#include <Render/Buffers/GpuBuffer.h>

namespace alexis
{
	class CommandContext;

	class ResourceManager
	{
	public:
		ResourceManager();
		TextureBuffer* GetTexture(const std::wstring& path);

	private:
		CommandContext* m_copyContext{ nullptr };

		using TextureMap = std::unordered_map<std::wstring, TextureBuffer>;
		TextureMap::iterator LoadTexture(const std::wstring& path);
		TextureMap m_textures;
	};

}