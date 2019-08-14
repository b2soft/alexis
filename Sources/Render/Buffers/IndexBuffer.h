#pragma once

#include "Buffer.h"

class IndexBuffer : public Buffer
{
public:
	IndexBuffer(const std::wstring& name = L"Index Buffer");
	virtual ~IndexBuffer() = default;

	virtual void CreateViews(size_t numElements, size_t elementSize) override;

	D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
	{
		return m_view;
	}

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc /* = nullptr */) const override
	{
		throw std::exception("IndexBuffer::GetSRV should not be called!");
	}
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc /* = nullptr */) const override
	{
		throw std::exception("IndexBuffer::GetUAV should not be called!");
	}

	size_t GetNumIndices() const
	{
		return m_numIndices;
	}

	size_t GetIndexFormat() const
	{
		return m_indexFormat;
	}

private:
	size_t m_numIndices;
	DXGI_FORMAT m_indexFormat;

	D3D12_INDEX_BUFFER_VIEW m_view;
};