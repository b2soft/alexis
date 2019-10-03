#pragma once

#include "CommandList.h"
#include <DirectXMath.h>
#include <d3d12.h>

#include <wrl.h>

#include <memory>
#include <vector>

#include "Buffers/IndexBuffer.h"
#include "Buffers/VertexBuffer.h"

struct VertexPositionDef
{
	VertexPositionDef() {}

	VertexPositionDef(const DirectX::XMFLOAT3& position)
		: Position(position)
	{
	}

	VertexPositionDef(DirectX::FXMVECTOR position)
	{
		XMStoreFloat3(&Position, position);
	}


	DirectX::XMFLOAT3 Position;
	//DirectX::XMFLOAT3 normal;

	static const int InputElementCount = 1;
	static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

using VertexCollection = std::vector<VertexPositionDef>;
using IndexCollection = std::vector<uint16_t>;

class Mesh
{
public:
	Mesh() = default;
	Mesh(const Mesh& copy) = delete;

	void Draw(CommandList& commandList);

	static std::unique_ptr<Mesh> LoadFBXFromFile(CommandList& commandList, const std::wstring& path);

private:
	friend struct std::default_delete<Mesh>;

	virtual ~Mesh() = default;

	void Initialize(CommandList& commandList, VertexCollection& vertices, IndexCollection& indices);

	VertexBuffer m_vertexBuffer;
	IndexBuffer m_indexBuffer;
	UINT m_indexCount{ 0 };
};