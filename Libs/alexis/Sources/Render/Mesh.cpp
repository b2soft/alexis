#include <Precompiled.h>

#include "Mesh.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <Render/CommandContext.h>

#include <Core/Core.h>
#include <Render/CommandManager.h>
#include <Render/Render.h>

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
		auto vertexBufferView = m_vertexBuffer.GetVertexBufferView();
		auto indexBufferView = m_indexBuffer.GetIndexBufferView();
		commandContext->List->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandContext->List->IASetVertexBuffers(0, 1, &vertexBufferView);
		commandContext->List->IASetIndexBuffer(&indexBufferView);
		commandContext->DrawIndexedInstanced(m_indexCount);
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
		if (vertices.size() >= UINT_MAX)
		{
			throw std::exception("Too many vertices for 32-bit index buffer!");
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