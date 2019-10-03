#include <Precompiled.h>

#include "../Application.h"

#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

using namespace DirectX;
using namespace Microsoft::WRL;

const D3D12_INPUT_ELEMENT_DESC VertexPositionDef::InputElements[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
};

void Mesh::Draw(CommandList& commandList)
{
	commandList.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList.SetVertexBuffer(0, m_vertexBuffer);
	commandList.SetIndexBuffer(m_indexBuffer);
	commandList.DrawIndexed(m_indexCount);
}

std::unique_ptr<Mesh> Mesh::LoadFBXFromFile(CommandList& commandList, const std::wstring& path)
{
	Assimp::Importer importer;

	std::string convertedPath(path.begin(), path.end());

	auto scene = importer.ReadFile(convertedPath, aiProcess_ConvertToLeftHanded |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
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

	for (std::size_t i = 0; i < scene->mNumMeshes; ++i)
	{
		const aiMesh* mesh = scene->mMeshes[i];

		for (unsigned int t = 0; t < mesh->mNumFaces; ++t)
		{
			const aiFace* face = &mesh->mFaces[t];

			indices.emplace_back(face->mIndices[0]);
			indices.emplace_back(face->mIndices[1]);
			indices.emplace_back(face->mIndices[2]);
		}

		if (mesh->HasPositions())
		{
			for (std::size_t vertexId = 0; vertexId < mesh->mNumVertices; ++vertexId)
			{
				VertexPositionDef def;
				def.Position = XMFLOAT3{ mesh->mVertices[vertexId].x, mesh->mVertices[vertexId].y, mesh->mVertices[vertexId].z };
				vertices.emplace_back(def);
			}
		}
	}

	std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
	mesh->Initialize(commandList, vertices, indices);

	return mesh;
}

void Mesh::Initialize(CommandList& commandList, VertexCollection& vertices, IndexCollection& indices)
{
	if (vertices.size() >= USHRT_MAX)
	{
		throw std::exception("Too many vertices for 16-bit index buffer!");
	}

	commandList.CopyVertexBuffer(m_vertexBuffer, vertices);
	commandList.CopyIndexBuffer(m_indexBuffer, indices);

	m_indexCount = static_cast<UINT>(indices.size());
}
