#include <Precompiled.h>

#include "ResourceManager.h"

#include <Core/Render.h>
#include <Core/CommandManager.h>
#include <Render/CommandContext.h>

// Mesh
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

// Material
#include <json.hpp>
#include <fstream>
#include <Render/Materials/PBRMaterial.h>
#include <Render/Materials/PBS_Simple.h>

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

	Mesh* ResourceManager::GetMesh(const std::wstring& path)
	{
		auto it = m_meshes.find(path);

		if (it != m_meshes.end())
		{
			return it->second.get();
		}

		it = LoadMesh(path);
		return it->second.get();
	}

	MaterialBase* ResourceManager::GetMaterial(const std::wstring& path)
	{
		auto it = m_materials.find(path);

		if (it != m_materials.end())
		{
			return it->second.get();
		}

		it = LoadMaterial(path);
		return it->second.get();
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

	ResourceManager::MeshMap::iterator ResourceManager::LoadMesh(const std::wstring& path)
	{
		if (path == L"$FS_QUAD")
		{
			std::unique_ptr<Mesh> mesh = Mesh::FullScreenQuad(m_copyContext);
			m_copyContext->Flush(true);

			return m_meshes.emplace(path, std::move(mesh)).first;
		}

		Assimp::Importer importer;

		std::string convertedPath(path.begin(), path.end());

		auto scene = importer.ReadFile(convertedPath, aiProcess_ConvertToLeftHanded |
			aiProcess_RemoveRedundantMaterials |
			aiProcess_CalcTangentSpace |
			aiProcess_Triangulate |
			//aiProcess_FlipUVs |
			aiProcess_JoinIdenticalVertices |
			aiProcess_ValidateDataStructure |
			aiProcess_PreTransformVertices);

		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			std::string errorStr = "Failed to load " + convertedPath + " with error: " + importer.GetErrorString();
			throw std::exception(errorStr.c_str());
		}

		using namespace DirectX;

		VertexCollection vertices;
		IndexCollection indices;

		uint32_t offset = 0;

		for (std::size_t i = 0; i < scene->mNumMeshes; ++i)
		{
			const aiMesh* mesh = scene->mMeshes[i];

			for (unsigned int t = 0; t < mesh->mNumFaces; ++t)
			{
				const aiFace* face = &mesh->mFaces[t];

				indices.emplace_back(offset + face->mIndices[0]);
				indices.emplace_back(offset + face->mIndices[1]);
				indices.emplace_back(offset + face->mIndices[2]);
			}

			vertices.reserve(mesh->mNumVertices);

			if (!mesh->HasPositions() || !mesh->HasNormals() || !mesh->HasTangentsAndBitangents() || !mesh->HasTextureCoords(0))
			{
				std::string errorStr = "Failed to load " + convertedPath + " : invalid model";
				throw std::exception(errorStr.c_str());
			}

			for (std::size_t vertexId = 0; vertexId < mesh->mNumVertices; ++vertexId)
			{
				VertexDef def;
				const auto& vertexPos = mesh->mVertices[vertexId];
				def.Position = XMFLOAT3{ vertexPos.x, vertexPos.y,vertexPos.z };

				const auto& normal = mesh->mNormals[vertexId];
				def.Normal = XMFLOAT3{ normal.x, normal.y, normal.z };

				const auto& tangent = mesh->mTangents[vertexId];
				def.Tangent = XMFLOAT3{ tangent.x, tangent.y, tangent.z };

				const auto& bitangent = mesh->mBitangents[vertexId];
				def.Bitangent = XMFLOAT3{ bitangent.x, bitangent.y, bitangent.z };

				const auto& uv0 = mesh->mTextureCoords[0][vertexId];
				def.UV0 = XMFLOAT2{ uv0.x, uv0.y };

				vertices.emplace_back(def);
			}

			offset += mesh->mNumVertices;
		}

		std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
		mesh->Initialize(m_copyContext, vertices, indices);

		m_copyContext->Flush(true);

		return m_meshes.emplace(path, std::move(mesh)).first;
	}

	ResourceManager::MaterialMap::iterator ResourceManager::LoadMaterial(const std::wstring& path)
	{
		fs::path filePath(path);

		if (!fs::exists(filePath))
		{
			throw std::exception("File not found!");
		}

		using json = nlohmann::json;

		std::ifstream ifs(path);
		json j = nlohmann::json::parse(ifs);

		std::string type = j["type"];
		if (type == "PBS")
		{
			PBRMaterialParams params;
			params.BaseColor = GetTexture(ToWStr(j["baseColor"]));
			params.NormalMap = GetTexture(ToWStr(j["normalMap"]));
			params.MetalRoughness = GetTexture(ToWStr(j["metalRoughness"]));

			auto material = std::make_unique<PBRMaterial>(params);
			return m_materials.emplace(path, std::move(material)).first;
		}
		else if (type == "PBS_Simple")
		{
			PBSMaterialParams params;
			params.BaseColor = GetTexture(ToWStr(j["baseColor"]));
			params.Metallic = j["metallic"];
			params.Roughness = j["roughness"];

			auto material = std::make_unique<PBSSimple>(params);
			return m_materials.emplace(path, std::move(material)).first;
		}

		return m_materials.end();
	}

}