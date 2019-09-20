#include <Precompiled.h>

#include "../Application.h"

#include "Mesh.h"

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

std::unique_ptr<Mesh> Mesh::CreateCube(CommandList& commandList, float size /*= 1.0f*/, bool rhcoords /*= false*/)
{
	const int k_faceCount = 6;

	static const XMVECTORF32 faceNormals[k_faceCount] =
	{
		{ 0, 0, 1 },
		{ 0, 0, -1 },
		{ 1, 0, 0 },
		{ -1, 0, 0 },
		{ 0, 1, 0 },
		{ 0, -1, 0 },
	};

	static const XMVECTORF32 textureCoordinates[4] =
	{
		{ 1, 0 },
		{ 1, 1 },
		{ 0, 1 },
		{ 0, 0 },
	};

	VertexCollection vertices;
	IndexCollection indices;

	size /= 2.0f;

	for (int i = 0; i < k_faceCount; i++)
	{
		XMVECTOR normal = faceNormals[i];

		XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

		XMVECTOR side1 = XMVector3Cross(normal, basis);
		XMVECTOR side2 = XMVector3Cross(normal, side1);

		// Six indices per face
		size_t vbase = vertices.size();
		indices.push_back(static_cast<uint16_t>(vbase + 0));
		indices.push_back(static_cast<uint16_t>(vbase + 1));
		indices.push_back(static_cast<uint16_t>(vbase + 2));

		indices.push_back(static_cast<uint16_t>(vbase + 0));
		indices.push_back(static_cast<uint16_t>(vbase + 2));
		indices.push_back(static_cast<uint16_t>(vbase + 3));

		// Four vertices per face
		vertices.push_back(VertexPositionDef((normal - side1 - side2) * size));//, normal, textureCoordinates[0]));
		vertices.push_back(VertexPositionDef((normal - side1 + side2) * size));//, normal, textureCoordinates[1]));
		vertices.push_back(VertexPositionDef((normal + side1 + side2) * size));//, normal, textureCoordinates[2]));
		vertices.push_back(VertexPositionDef((normal + side1 - side2) * size));//, normal, textureCoordinates[3]));
	}

	std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
	mesh->Initialize(commandList, vertices, indices, rhcoords);

	return mesh;
}

void Mesh::Initialize(CommandList& commandList, VertexCollection& vertices, IndexCollection& indices, bool rhcoords)
{
	if (vertices.size() >= USHRT_MAX)
	{
		throw std::exception("Too many vertices for 16-bit index buffer!");
	}

	if (!rhcoords)
	{
		//ReverseWinding
	}

	commandList.CopyVertexBuffer(m_vertexBuffer, vertices);
	commandList.CopyIndexBuffer(m_indexBuffer, indices);

	m_indexCount = static_cast<UINT>(indices.size());
}
