#include <Precompiled.h>

#include "../../Application.h"
#include "../ResourceStateTracker.h"

#include "StructuredBuffer.h"

StructuredBuffer::StructuredBuffer(const std::wstring& name /*= L"Structured Buffer"*/)
	: Buffer(name)
	, m_counterBuffer(CD3DX12_RESOURCE_DESC::Buffer(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), 1, 4, name + L"Counter")
	, m_numElements(0)
	, m_elementSize(0)
{
	m_srv = Application::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_uav = Application::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

StructuredBuffer::StructuredBuffer(const D3D12_RESOURCE_DESC& resDesc, size_t numElements, size_t elementSize, const std::wstring& name /*= L"Structured Buffer"*/)
	: Buffer(resDesc, numElements, elementSize, name)
	, m_counterBuffer(CD3DX12_RESOURCE_DESC::Buffer(4, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS), 1, 4, name + L"Counter")
	, m_numElements(0)
	, m_elementSize(0)
{
	m_srv = Application::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_uav = Application::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void StructuredBuffer::CreateViews(size_t numElements, size_t elementSize)
{
	auto device = Application::Get().GetDevice();

	m_numElements = numElements;
	m_elementSize = elementSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = static_cast<UINT>(m_numElements);
	srvDesc.Buffer.StructureByteStride = static_cast<UINT>(m_elementSize);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(m_d3d12Resource.Get(), &srvDesc, m_srv.GetDescriptorHandle());

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.NumElements = static_cast<UINT>(m_numElements);
	uavDesc.Buffer.StructureByteStride = static_cast<UINT>(m_elementSize);
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	device->CreateUnorderedAccessView(m_d3d12Resource.Get(), m_counterBuffer.GetD3D12Resource().Get(), &uavDesc, m_uav.GetDescriptorHandle());
}
