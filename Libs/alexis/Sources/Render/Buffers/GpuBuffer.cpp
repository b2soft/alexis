#include <Precompiled.h>

#include "GpuBuffer.h"

#include <Render/Render.h>

namespace alexis
{
	void GpuBuffer::Create(std::size_t numElements, std::size_t elementsSize, const D3D12_CLEAR_VALUE* clearValue)
	{
		if (m_clearValue)
		{
			m_clearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
		}

		m_numElements = numElements;
		m_elementSize = elementsSize;

		m_bufferSize = m_numElements * m_elementSize;

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


		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_bufferSize);

		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			clearValue,
			IID_PPV_ARGS(&m_resource)));

		CreateViews();
	}

	void GpuBuffer::Reset()
	{
		m_resource.Reset();
		m_clearValue.reset();
	}

	void GpuBuffer::SetFromResource(ID3D12Resource* resource)
	{
		m_resource = resource;

		if (m_resource)
		{
			CreateViews();
		}
	}

	void VertexBuffer::CreateViews()
	{
		m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
		m_view.StrideInBytes = static_cast<UINT>(m_elementSize);//TODO
		m_view.SizeInBytes = static_cast<UINT>(m_bufferSize);
	}

	void IndexBuffer::CreateViews()
	{
		m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
		m_view.Format = (m_elementSize == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		m_view.SizeInBytes = static_cast<UINT>(m_bufferSize);
	}

	void ConstantBuffer::Create(std::size_t numElements, std::size_t elementSize)
	{
		m_numElements = numElements;
		m_elementSize = elementSize;

		m_bufferSize = Math::AlignUp(m_numElements * m_elementSize, 256);

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

		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_bufferSize);

		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
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

	void TextureBuffer::Create(uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t numMips /*= 1*/, const D3D12_CLEAR_VALUE* clearValue /*= nullptr*/)
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

		Create(textureDesc, clearValue);
	}

	void TextureBuffer::Create(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue /*= nullptr*/)
	{
		if (m_clearValue)
		{
			m_clearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
		}

		auto device = Render::GetInstance()->GetDevice();

		auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		ThrowIfFailed(device->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&m_resource)));

		CreateViews();
	}

	void TextureBuffer::CreateFromSwapchain(ID3D12Resource* resource)
	{
		m_resource.Attach(resource);
	}

	void TextureBuffer::Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize /*= 1*/)
	{
		if (m_resource)
		{
			CD3DX12_RESOURCE_DESC resDesc(m_resource->GetDesc());

			resDesc.Width = std::max(width, 1u);
			resDesc.Height = std::max(height, 1u);
			resDesc.DepthOrArraySize = depthOrArraySize;

			auto device = Render::GetInstance()->GetDevice();

			auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

			ThrowIfFailed(device->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resDesc,
				D3D12_RESOURCE_STATE_COMMON,
				m_clearValue.get(),
				IID_PPV_ARGS(&m_resource)
			));

			CreateViews();
		}
	}

	void TextureBuffer::CreateViews()
	{
		auto resDesc = m_resource->GetDesc();

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		if (resDesc.Format == DXGI_FORMAT_R24G8_TYPELESS)
		{
			srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		}
		else
		{
			srvDesc.Format = resDesc.Format;
		}

		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = resDesc.MipLevels;

		auto render = Render::GetInstance();
		auto device = render->GetDevice();

		D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport{ resDesc.Format };
		device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(D3D12_FEATURE_DATA_FORMAT_SUPPORT));

		const bool rtFormatSupport = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0;
		const bool dsFormatSupport = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0;
		const bool srvFormatSupport = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE) != 0;

		if (/*srvFormatSupport*/ true)
		{
			m_srv = render->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
			device->CreateShaderResourceView(m_resource.Get(), &srvDesc, m_srv.GetDescriptorHandle());
		}

		if ((resDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) != 0 /*&& rtFormatSupport*/)
		{
			m_rtv = render->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			device->CreateRenderTargetView(m_resource.Get(), nullptr, m_rtv.GetDescriptorHandle());
		}

		if ((resDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) != 0 /*&& dsFormatSupport*/)
		{
			if (resDesc.Format == DXGI_FORMAT_R24G8_TYPELESS)
			{
				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

				m_dsv = render->AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
				device->CreateDepthStencilView(m_resource.Get(), &dsvDesc, m_dsv.GetDescriptorHandle());
			}
		}
	}

}