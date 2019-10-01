#pragma once

#include "Buffer.h"
#include "../DescriptorAllocation.h"

#include "../../d3dx12.h"

class RawBuffer : public Buffer
{
public:
	RawBuffer(const std::wstring& name = L"Raw Buffer");
	RawBuffer(const D3D12_RESOURCE_DESC& resDesc, size_t numElements, size_t elementSize, const std::wstring& name = L"Raw Buffer");

	size_t GetBufferSize() const
	{
		return m_bufferSize;
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

private:
	size_t m_bufferSize;

	DescriptorAllocation m_srv;
	DescriptorAllocation m_uav;
};