#pragma once

#include "RawBuffer.h"

class StructuredBuffer : public Buffer
{
public:
	StructuredBuffer(const std::wstring& name = L"Structured Buffer");
	StructuredBuffer(const D3D12_RESOURCE_DESC& resDesc, size_t numElements, size_t elementSize, const std::wstring& name = L"Structured Buffer");

	virtual size_t GetNumElements() const
	{
		return m_numElements;
	}

	virtual size_t GetElementSize() const
	{
		return m_elementSize;
	}

	virtual void CreateViews(size_t numElements, size_t elementSize) override;

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override
	{
		return m_srv.GetDescriptorHandle();
	}

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override
	{
		return m_uav.GetDescriptorHandle();
	}

	const RawBuffer& GetCounterBuffer() const
	{
		return m_counterBuffer;
	}

private:
	size_t m_numElements;
	size_t m_elementSize;

	DescriptorAllocation m_srv;
	DescriptorAllocation m_uav;

	// A buffer to store the internal counter for the structured buffer.
	RawBuffer m_counterBuffer;
};