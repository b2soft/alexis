#include <Precompiled.h>

#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <Render/CommandContext.h>

#include <Core/Core.h>
#include <Core/CommandManager.h>
#include <Core/Render.h>

using namespace DirectX;
using namespace Microsoft::WRL;

namespace alexis
{
	const D3D12_INPUT_ELEMENT_DESC VertexDef::InputElements[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	void Mesh::Draw(CommandContext* commandContext)
	{
		// todo: bundle it?
		commandContext->List->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandContext->List->IASetVertexBuffers(0, 1, &m_vertexBuffer.GetVertexBufferView());
		commandContext->List->IASetIndexBuffer(&m_indexBuffer.GetIndexBufferView());
		commandContext->DrawIndexedInstanced(m_indexCount);
	}

	std::unique_ptr<alexis::Mesh> Mesh::LoadFBXFromFile(const std::wstring& path)
	{
		//TODO replace with res manager
		auto commandManager = Render::GetInstance()->GetCommandManager();
		CommandContext* commandContext = commandManager->CreateCommandContext();

		Assimp::Importer importer;

		std::string convertedPath(path.begin(), path.end());

		auto scene = importer.ReadFile(convertedPath, /*aiProcess_ConvertToLeftHanded |*/
			aiProcess_RemoveRedundantMaterials |
			/*aiProcess_CalcTangentSpace |*/
			//aiProcess_Triangulate |
			//aiProcess_FlipUVs |
			//aiProcess_JoinIdenticalVertices |
			aiProcess_ValidateDataStructure /*|
			aiProcess_PreTransformVertices*/);

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
		}

		std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
		mesh->Initialize(commandContext, vertices, indices);

		commandContext->Finish(true);

		return mesh;
	}

	std::unique_ptr<alexis::Mesh> Mesh::FullScreenQuad(CommandContext* commandContext)
	{
		VertexDef defs[4];

		defs[0].Position = XMFLOAT3(-1.0f, -1.0f, 0.5f);
		defs[0].UV0 = XMFLOAT2(0.0f, 1.0f);

		defs[1].Position = XMFLOAT3(-1.0f, 1.0f, 0.5f);
		defs[1].UV0 = XMFLOAT2(0.0f, 0.0f);

		defs[2].Position = XMFLOAT3(1.0f, -1.0f, 0.5f);
		defs[2].UV0 = XMFLOAT2(1.0f, 1.0f);

		defs[3].Position = XMFLOAT3(1.0f, 1.0f, 0.5f);
		defs[3].UV0 = XMFLOAT2(1.0f, 0.0f);

		VertexCollection vertices = { defs[0],defs[1], defs[2], defs[3] };
		IndexCollection indices = { 0, 1, 2, 2, 1, 3 };

		std::unique_ptr<Mesh> fsQuad = std::make_unique<Mesh>();
		fsQuad->Initialize(commandContext, vertices, indices);

		return fsQuad;
	}

	void Mesh::Initialize(CommandContext* commandContext, VertexCollection& vertices, IndexCollection& indices)
	{
		if (vertices.size() >= USHRT_MAX)
		{
			throw std::exception("Too many vertices for 16-bit index buffer!");
		}
		m_indexCount = static_cast<UINT>(indices.size());

		m_vertexBuffer.Create(vertices.size(), sizeof(VertexDef));
		m_indexBuffer.Create(m_indexCount, sizeof(uint16_t));

		//Todo : remove element size duplication?

		commandContext->CopyBuffer(m_vertexBuffer, vertices.data(), vertices.size(), sizeof(VertexDef));
		commandContext->CopyBuffer(m_indexBuffer, indices.data(), m_indexCount, sizeof(uint16_t));

		//commandContext->TransitionResource(m_vertexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		//commandContext->TransitionResource(m_indexBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	}

}