#pragma once

#include "Buffer.h"

class VertexBuffer : public Buffer
{
public:
	VertexBuffer(const std::wstring& name = L"Vertex Buffer");
	virtual ~VertexBuffer() = default;

	virtual void CreateViews(size_t numElements, size_t elementSize) override;

	D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
	{
		return m_view;
	}

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc /* = nullptr */) const override
	{
		throw std::exception("VertexBuffer::GetSRV should not be called!");
	}
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc /* = nullptr */) const override
	{
		throw std::exception("VertexBuffer::GetUAV should not be called!");
	}

	size_t GetNumVertices() const
	{
		return m_numVertices;
	}

	size_t GetVertexStride() const
	{
		return m_vertexStride;
	}

private:
	size_t m_numVertices;
	size_t m_vertexStride;

	D3D12_VERTEX_BUFFER_VIEW m_view;
};
