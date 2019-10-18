#include <Precompiled.h>

#include "GpuBuffer.h"

#include <Core/Render.h>

namespace alexis
{
	void GpuBuffer::Create(std::size_t numElements, std::size_t elementsSize)
	{
		m_numElements = numElements;
		m_elementsSize = elementsSize;

		m_bufferSize = m_numElements * m_elementsSize;

		D3D12_RESOURCE_DESC Desc = {};
		Desc.Alignment = 0;
		Desc.DepthOrArraySize = 1;
		Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		Desc.Format = DXGI_FORMAT_UNKNOWN;
		Desc.Height = 1;
		Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Desc.MipLevels = 1;
		Desc.SampleDesc.Count = 1;
		Desc.SampleDesc.Quality = 0;
		Desc.Width = m_bufferSize;

		auto device = Render::GetInstance()->GetDevice();

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_bufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_resource)));

		CreateViews();
	}

	void GpuBuffer::SetFromResource(ID3D12Resource* resource)
	{
		m_resource = resource;

		CreateViews();
	}

	D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetVertexBufferView() const
	{
		return m_view;
	}

	void VertexBuffer::CreateViews()
	{
		m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
		m_view.StrideInBytes = static_cast<UINT>(m_elementsSize);//TODO
		m_view.SizeInBytes = static_cast<UINT>(m_bufferSize);
	}

	void ConstantBuffer::Create(std::size_t numElements, std::size_t elementsSize)
	{
		m_numElements = numElements;
		m_elementsSize = elementsSize;

		m_bufferSize = Math::AlignUp(m_numElements * m_elementsSize, 256);

		D3D12_RESOURCE_DESC Desc = {};
		Desc.Alignment = 0;
		Desc.DepthOrArraySize = 1;
		Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		Desc.Format = DXGI_FORMAT_UNKNOWN;
		Desc.Height = 1;
		Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Desc.MipLevels = 1;
		Desc.SampleDesc.Count = 1;
		Desc.SampleDesc.Quality = 0;
		Desc.Width = m_bufferSize;

		auto device = Render::GetInstance()->GetDevice();

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_bufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_resource)));

		CreateViews();
	}

	void ConstantBuffer::CreateViews()
	{
		auto resDesc = m_resource->GetDesc();

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = m_resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = static_cast<UINT>(m_bufferSize); // Already aligned

		auto render = Render::GetInstance();
		auto device = render->GetDevice();
		m_cbv = render->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

		device->CreateConstantBufferView(&cbvDesc, m_cbv.GetDescriptorHandle());
	}

	void TextureBuffer::Create(uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t numMips /*= 1*/)
	{
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = numMips;
		textureDesc.Format = format;
		textureDesc.Width = width;
		textureDesc.Height = height;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		auto device = Render::GetInstance()->GetDevice();

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_resource)));

		CreateViews();
	}

	void TextureBuffer::CreateViews()
	{
		auto resDesc = m_resource->GetDesc();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = resDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = resDesc.MipLevels;

		auto render = Render::GetInstance();
		auto device = render->GetDevice();
		m_srv = render->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

		device->CreateShaderResourceView(m_resource.Get(), &srvDesc, m_srv.GetDescriptorHandle());
	}

	void DynamicConstantBuffer::Create(std::size_t numElements, std::size_t elementsSize)
	{
		m_numElements = numElements;
		m_elementsSize = elementsSize;

		m_bufferSize = Math::AlignUp(m_numElements * m_elementsSize, 256);

		D3D12_RESOURCE_DESC Desc = {};
		Desc.Alignment = 0;
		Desc.DepthOrArraySize = 1;
		Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		Desc.Format = DXGI_FORMAT_UNKNOWN;
		Desc.Height = 1;
		Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Desc.MipLevels = 1;
		Desc.SampleDesc.Count = 1;
		Desc.SampleDesc.Quality = 0;
		Desc.Width = m_bufferSize;

		auto device = Render::GetInstance()->GetDevice();

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(m_bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_resource)
		));

		m_resource->Map(0, nullptr, &m_cpuPtr);
		m_gpuPtr = m_resource->GetGPUVirtualAddress();

		auto resDesc = m_resource->GetDesc();

		m_cbv = Render::GetInstance()->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = m_gpuPtr;
		cbvDesc.SizeInBytes = static_cast<UINT>(m_bufferSize);

		device->CreateConstantBufferView(&cbvDesc, m_cbv.GetDescriptorHandle());
	}

}