#include <Precompiled.h>

#include "VertexBuffer.h"

VertexBuffer::VertexBuffer(const std::wstring& name /*= L"Vertex Buffer"*/)
	: Buffer( name)
	, m_numVertices(0)
	, m_vertexStride(0)
	, m_view({})
{
}

void VertexBuffer::CreateViews(size_t numElements, size_t elementSize)
{
	m_numVertices = numElements;
	m_vertexStride = elementSize;

	m_view.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
	m_view.SizeInBytes = static_cast<UINT>(m_numVertices * m_vertexStride);
	m_view.StrideInBytes = static_cast<UINT>(m_vertexStride);
}
