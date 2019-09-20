#include <Precompiled.h>

#include "IndexBuffer.h"

IndexBuffer::IndexBuffer(const std::wstring& name /*= L"Index Buffer"*/)
	: Buffer(name)
	, m_numIndices(0)
	, m_indexFormat(DXGI_FORMAT_UNKNOWN)
	, m_view({})
{
}

void IndexBuffer::CreateViews(size_t numElements, size_t elementSize)
{
	assert(elementSize == 2 || elementSize == 4 && "Indices must be 16 or 32 bit integers only!");

	m_numIndices = numElements;
	m_indexFormat = (elementSize == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

	m_view.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
	m_view.SizeInBytes = static_cast<UINT>(numElements * elementSize);
	m_view.Format = m_indexFormat;
}
