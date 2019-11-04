#include <Precompiled.h>

#include "ResourceManager.h"

namespace alexis
{

	alexis::TextureBuffer& ResourceManager::GetTexture(const std::wstring& path)
	{
		auto it = m_textures.find(path);
		if (it != m_textures.end())
		{
			return it->second;
		}

		LoadTexture(path);
	}

	void ResourceManager::LoadTexture(const std::wstring& path)
	{
		
	}

}