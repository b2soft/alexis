#pragma once

#include <Utils/unordered_map.h>

#include <Render/Materials/MaterialBase.h>

#include <Render/Buffers/GpuBuffer.h>
#include <Render/Mesh.h>

namespace alexis
{
	class CommandContext;

	class ResourceManager
	{
	public:
		ResourceManager();

		TextureBuffer* GetTexture(std::wstring_view path);
		Mesh* GetMesh(std::wstring_view path);
		Material* GetMaterial(std::wstring_view path);

	private:
		CommandContext* m_copyContext{ nullptr };

		using TextureMap = alexis::unordered_map<std::wstring, TextureBuffer>;
		TextureMap::iterator LoadTexture(std::wstring_view path);
		TextureMap m_textures;

		using MeshMap = alexis::unordered_map<std::wstring, std::unique_ptr<Mesh>>;
		MeshMap::iterator LoadMesh(std::wstring_view path);
		MeshMap m_meshes;

		using MaterialMap = alexis::unordered_map<std::wstring, std::unique_ptr<Material>>;
		MaterialMap::iterator LoadMaterial(std::wstring_view path);
		MaterialMap m_materials;
	};

}