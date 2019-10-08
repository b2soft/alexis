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
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 Tangent;
	DirectX::XMFLOAT3 Bitangent;
	DirectX::XMFLOAT2 UV0;

	static const int InputElementCount = 5;
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
	static std::unique_ptr<Mesh> FullScreenQuad(CommandList& commandList);

private:
	friend struct std::default_delete<Mesh>;

	virtual ~Mesh() = default;

	void Initialize(CommandList& commandList, VertexCollection& vertices, IndexCollection& indices);

	VertexBuffer m_vertexBuffer;
	IndexBuffer m_indexBuffer;
	UINT m_indexCount{ 0 };
};