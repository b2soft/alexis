#pragma once

#include <unordered_map>

#include <Render/Buffers/GpuBuffer.h>

namespace alexis
{
	class ResourceManager
	{
	public:
		TextureBuffer& GetTexture(const std::wstring& path);

	private:
		void LoadTexture(const std::wstring& path);

		std::unordered_map<std::wstring, TextureBuffer> m_textures;

	};

}