#pragma once

#include <Render/Buffers/GpuBuffer.h>

namespace alexis
{
	struct VertexDef
	{
		DirectX::XMFLOAT3 Position;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT3 Tangent;
		DirectX::XMFLOAT3 Bitangent;
		DirectX::XMFLOAT2 UV0;

		static const int InputElementCount = 5;
		static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
	};

	using VertexCollection = std::vector<VertexDef>;
	using IndexCollection = std::vector<uint16_t>;

	class CommandContext;

	class Mesh
	{
	public:
		Mesh() = default;
		Mesh(const Mesh& copy) = delete;

		void Draw(CommandContext* commandContext);

		static std::unique_ptr<Mesh> LoadFBXFromFile(const std::wstring& path);
		static std::unique_ptr<Mesh> FullScreenQuad(CommandContext* commandContext);

	private:
		friend struct std::default_delete<Mesh>;

		void Initialize(CommandContext* commandContext, VertexCollection& vertices, IndexCollection& indices);

		VertexBuffer m_vertexBuffer;
		IndexBuffer m_indexBuffer;
		UINT m_indexCount{ 0 };
	};
}