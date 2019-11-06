#pragma once

#include <unordered_map>

#include <Render/Materials/PBRMaterial.h>

#include <Render/Buffers/GpuBuffer.h>
#include <Render/Mesh.h>

namespace alexis
{
	class CommandContext;

	class ResourceManager
	{
	public:
		ResourceManager();

		TextureBuffer* GetTexture(const std::wstring& path);
		Mesh* GetMesh(const std::wstring& path);

	private:
		CommandContext* m_copyContext{ nullptr };

		using TextureMap = std::unordered_map<std::wstring, TextureBuffer>;
		TextureMap::iterator LoadTexture(const std::wstring& path);
		TextureMap m_textures;

		using MeshMap = std::unordered_map<std::wstring, std::unique_ptr<Mesh>>;
		MeshMap::iterator LoadMesh(const std::wstring& path);
		MeshMap m_meshes;

	/*	using MaterialMap = std::unordered_map<std::wstring, IMaterial>;
		MaterialMap::iterator LoadMaterial(const std::wstring& path);
		MaterialMap m_materials;*/
	};

}