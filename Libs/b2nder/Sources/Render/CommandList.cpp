#include <Precompiled.h>

#include <cassert>

#include "CommandList.h"

#include "../Application.h"
#include "../CommandQueue.h"
#include "../Helpers.h"
#include "DynamicDescriptorHeap.h"
#include "Resource.h"
#include "ResourceStateTracker.h"
#include "RootSignature.h"
#include "UploadBuffer.h"
#include "RenderTarget.h"
#include "GenerateMipsPSO.h"

#include "Buffers/IndexBuffer.h"
#include "Buffers/VertexBuffer.h"
#include "Buffers/StructuredBuffer.h"
#include "Buffers/RawBuffer.h"

std::map<std::wstring, ID3D12Resource*> CommandList::s_textureCache;
std::mutex CommandList::s_textureCacheMutex;

CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type)
	: m_d3d12CommandListType(type)
{
	auto device = Application::Get().GetDevice();

	ThrowIfFailed(device->CreateCommandAllocator(m_d3d12CommandListType, IID_PPV_ARGS(&m_d3d12CommandAllocator)));

	ThrowIfFailed(device->CreateCommandList(0, m_d3d12CommandListType, m_d3d12CommandAllocator.Get(),
		nullptr, IID_PPV_ARGS(&m_d3d12CommandList)));

	m_uploadBuffer = std::make_unique<UploadBuffer>();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i] = std::make_unique<DynamicDescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
		m_descriptorHeaps[i] = nullptr;
	}
}

CommandList::~CommandList()
{}

void CommandList::TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource /*= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES*/, bool flushBarriers /*= false*/)
{

	D3D12_RESOURCE_BARRIER& barrierDesc = m_resourceBarrierBuffer[m_numBarriersToFlush++];

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = resource.Get();
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	barrierDesc.Transition.StateAfter = stateAfter;


	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource, bool flushBarriers)
{
	TransitionBarrier(resource.GetD3D12Resource(), stateAfter, subresource, flushBarriers);
}

void CommandList::AliasingBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> srcResource, Microsoft::WRL::ComPtr<ID3D12Resource> dstResource, bool flushBarriers /*= false*/)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(srcResource.Get(), dstResource.Get());

	m_resourceStateTracker->ResourceBarrier(barrier);

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::AliasingBarrier(const Resource& srcResource, const Resource& dstResource, bool flushBarriers /*= false*/)
{
	AliasingBarrier(srcResource.GetD3D12Resource(), dstResource.GetD3D12Resource(), flushBarriers);
}

void CommandList::UAVBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, bool flushBarriers /*= false*/)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(resource.Get());

	m_resourceStateTracker->ResourceBarrier(barrier);

	if (flushBarriers)
	{
		FlushResourceBarriers();
	}
}

void CommandList::UAVBarrier(const Resource& resource, bool flushBarriers /*= false*/)
{
	UAVBarrier(resource.GetD3D12Resource(), flushBarriers);
}

void CommandList::FlushResourceBarriers()
{
	m_resourceStateTracker->FlushResourceBarriers(*this);
}

void CommandList::CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> srcRes)
{
	TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

	FlushResourceBarriers();

	m_d3d12CommandList->CopyResource(dstRes.Get(), srcRes.Get());

	TrackResource(dstRes);
	TrackResource(srcRes);
}

void CommandList::CopyResource(Resource& dstRes, const Resource& srcRes)
{
	CopyResource(dstRes.GetD3D12Resource(), srcRes.GetD3D12Resource());
}

void CommandList::ResolveSubresource(Resource& dstRes, const Resource& srcRes, uint32_t dstSubresource, uint32_t srcSubresource)
{
	TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_RESOLVE_DEST, dstSubresource);
	TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, srcSubresource);

	FlushResourceBarriers();

	m_d3d12CommandList->ResolveSubresource(dstRes.GetD3D12Resource().Get(), dstSubresource, srcRes.GetD3D12Resource().Get(), srcSubresource, dstRes.GetD3D12ResourceDesc().Format);

	TrackResource(srcRes);
	TrackResource(dstRes);
}

void CommandList::CopyBuffer(Buffer& buffer, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
	auto device = Application::Get().GetDevice();

	size_t bufferSize = numElements * elementSize;

	Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource;
	if (bufferSize == 0)
	{
		// Null resource
	}
	else
	{
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&d3d12Resource)
		));

		// Add to global tracker
		ResourceStateTracker::AddGlobalResourceState(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

		if (bufferData)
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> uploadResource;
			ThrowIfFailed(device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadResource)
			));

			D3D12_SUBRESOURCE_DATA subresourceData = {};
			subresourceData.pData = bufferData;
			subresourceData.RowPitch = bufferSize;
			subresourceData.SlicePitch = subresourceData.RowPitch;

			m_resourceStateTracker->TransitionResource(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST);
			FlushResourceBarriers();

			UpdateSubresources(m_d3d12CommandList.Get(), d3d12Resource.Get(), uploadResource.Get(), 0, 0, 1, &subresourceData);

			TrackResource(uploadResource);
		}

		TrackResource(d3d12Resource);
	}

	buffer.SetD3D12Resource(d3d12Resource);
	buffer.CreateViews(numElements, elementSize);
}

void CommandList::CopyVertexBuffer(VertexBuffer& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData)
{
	CopyBuffer(vertexBuffer, numVertices, vertexStride, vertexBufferData);
}

void CommandList::CopyIndexBuffer(IndexBuffer& indexBuffer, size_t numIndices, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
	size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
	CopyBuffer(indexBuffer, numIndices, indexSizeInBytes, indexBufferData);
}

void CommandList::CopyRawBuffer(RawBuffer& rawBuffer, size_t bufferSize, const void* bufferData)
{
	CopyBuffer(rawBuffer, 1, bufferSize, bufferData, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

void CommandList::CopyStructuredBuffer(StructuredBuffer& structuredBuffer, size_t numElements, size_t elementSize, const void* bufferData)
{
	CopyBuffer(structuredBuffer, numElements, elementSize, bufferData, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
}

void CommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology)
{
	m_d3d12CommandList->IASetPrimitiveTopology(primitiveTopology);
}

void CommandList::SetViewport(const D3D12_VIEWPORT& viewport)
{
	SetViewports({ viewport });
}

void CommandList::SetViewports(const std::vector<D3D12_VIEWPORT>& viewports)
{
	assert(viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
	m_d3d12CommandList->RSSetViewports(static_cast<UINT>(viewports.size()),
		viewports.data());
}

void CommandList::SetScissorRect(const D3D12_RECT& scissorRect)
{
	SetScissorRects({ scissorRect });
}

void CommandList::SetScissorRects(const std::vector<D3D12_RECT>& scissorRects)
{
	assert(scissorRects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE);
	m_d3d12CommandList->RSSetScissorRects(static_cast<UINT>(scissorRects.size()),
		scissorRects.data());
}

void CommandList::SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState)
{
	m_d3d12CommandList->SetPipelineState(pipelineState.Get());

	TrackResource(pipelineState);
}

void CommandList::SetGraphicsRootSignature(const RootSignature& rootSignature)
{
	auto d3d12RootSignature = rootSignature.GetRootSignature().Get();
	if (m_rootSignature != d3d12RootSignature)
	{
		m_rootSignature = d3d12RootSignature;

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_dynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
		}

		m_d3d12CommandList->SetGraphicsRootSignature(m_rootSignature);

		TrackResource(m_rootSignature);
	}
}

void CommandList::SetComputeRootSignature(const RootSignature& rootSignature)
{
	auto d3d12RootSignature = rootSignature.GetRootSignature().Get();
	if (m_rootSignature != d3d12RootSignature)
	{
		m_rootSignature = d3d12RootSignature;

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_dynamicDescriptorHeap[i]->ParseRootSignature(rootSignature);
		}

		m_d3d12CommandList->SetComputeRootSignature(m_rootSignature);

		TrackResource(m_rootSignature);
	}
}

void CommandList::ClearTexture(const Texture& texture, const float clearColor[4])
{
	TransitionBarrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_d3d12CommandList->ClearRenderTargetView(texture.GetRTV(), clearColor, 0, nullptr);

	TrackResource(texture);
}

void CommandList::ClearDepthStencilTexture(const Texture& texture, D3D12_CLEAR_FLAGS clearFlags, float depth /*= 1.0f*/, uint8_t stencil /*= 0*/)
{
	TransitionBarrier(texture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_d3d12CommandList->ClearDepthStencilView(texture.GetDSV(), clearFlags, depth, stencil, 0, nullptr);

	TrackResource(texture);
}

void CommandList::CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData)
{
	auto device = Application::Get().GetDevice();
	auto destinationResource = texture.GetD3D12Resource();
	if (destinationResource)
	{
		// Resource must be in the copy-destination state.
		TransitionBarrier(texture, D3D12_RESOURCE_STATE_COPY_DEST);
		FlushResourceBarriers();

		UINT64 requiredSize = GetRequiredIntermediateSize(destinationResource.Get(), firstSubresource, numSubresources);

		// Create a temporary (intermediate) resource for uploading the subresources
		ComPtr<ID3D12Resource> intermediateResource;
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(requiredSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&intermediateResource)
		));

		UpdateSubresources(m_d3d12CommandList.Get(), destinationResource.Get(), intermediateResource.Get(), 0, firstSubresource, numSubresources, subresourceData);

		TrackResource(intermediateResource);
		TrackResource(destinationResource);
	}
}

void CommandList::SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
	// Constant buffers must be 256-byte aligned
	auto heapAllococation = m_uploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	memcpy(heapAllococation.cpu, bufferData, sizeInBytes);

	m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllococation.gpu);
}

void CommandList::SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
	m_d3d12CommandList->SetGraphicsRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void CommandList::SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants)
{
	m_d3d12CommandList->SetComputeRoot32BitConstants(rootParameterIndex, numConstants, constants, 0);
}

void CommandList::SetVertexBuffer(uint32_t slot, const VertexBuffer& vertexBuffer)
{
	TransitionBarrier(vertexBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	auto vertexBufferView = vertexBuffer.GetVertexBufferView();

	m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);

	TrackResource(vertexBuffer);
}

void CommandList::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData)
{
	size_t bufferSize = numVertices * vertexSize;

	auto heapAllocation = m_uploadBuffer->Allocate(bufferSize, vertexSize);
	memcpy(heapAllocation.cpu, vertexBufferData, bufferSize);

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
	vertexBufferView.BufferLocation = heapAllocation.gpu;
	vertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	vertexBufferView.StrideInBytes = static_cast<UINT>(vertexSize);

	m_d3d12CommandList->IASetVertexBuffers(slot, 1, &vertexBufferView);
}

void CommandList::SetIndexBuffer(const IndexBuffer& indexBuffer)
{
	TransitionBarrier(indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	auto indexBufferView = indexBuffer.GetIndexBufferView();

	m_d3d12CommandList->IASetIndexBuffer(&indexBufferView);

	TrackResource(indexBuffer);
}

void CommandList::SetDynamicIndexBuffer(size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData)
{
	size_t indexSizeInBytes = indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4;
	size_t bufferSize = numIndicies * indexSizeInBytes;

	auto heapAllocation = m_uploadBuffer->Allocate(bufferSize, indexSizeInBytes);
	memcpy(heapAllocation.cpu, indexBufferData, bufferSize);

	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	indexBufferView.BufferLocation = heapAllocation.gpu;
	indexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
	indexBufferView.Format = indexFormat;

	m_d3d12CommandList->IASetIndexBuffer(&indexBufferView);
}

void CommandList::SetGraphicsDynamicStructuredBuffer(uint32_t slot, size_t numElements, size_t elementSize, const void* bufferData)
{
	size_t bufferSize = numElements * elementSize;

	auto heapAllocation = m_uploadBuffer->Allocate(bufferSize, elementSize);

	memcpy(heapAllocation.cpu, bufferData, bufferSize);

	m_d3d12CommandList->SetGraphicsRootShaderResourceView(slot, heapAllocation.gpu);
}

void CommandList::SetShaderResourceView(uint32_t rootParameterIndex,
	uint32_t descriptorOffset,
	const Resource& resource,
	D3D12_RESOURCE_STATES stateAfter,
	UINT firstSubresource,
	UINT numSubresources,
	const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
{
	if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (uint32_t i = 0; i < numSubresources; ++i)
		{
			TransitionBarrier(resource, stateAfter, firstSubresource + i);
		}
	}
	else
	{
		TransitionBarrier(resource, stateAfter);
	}

	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, resource.GetSRV(srv));

	TrackResource(resource);
}

void CommandList::SetUnorderedAccessView(uint32_t rootParameterIndex,
	uint32_t descrptorOffset,
	const Resource& resource,
	D3D12_RESOURCE_STATES stateAfter,
	UINT firstSubresource,
	UINT numSubresources,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav)
{
	if (numSubresources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
	{
		for (uint32_t i = 0; i < numSubresources; ++i)
		{
			TransitionBarrier(resource, stateAfter, firstSubresource + i);
		}
	}
	else
	{
		TransitionBarrier(resource, stateAfter);
	}

	m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descrptorOffset, 1, resource.GetUAV(uav));

	TrackResource(resource);
}

void CommandList::SetRenderTarget(const RenderTarget& renderTarget)
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> renderTargetDescriptors;
	renderTargetDescriptors.reserve(AttachmentPoint::NumAttachmentPoints);

	// Bind color targets
	const auto& textures = renderTarget.GetTextures();

	for (int i = 0; i < AttachmentPoint::DepthStencil; ++i)
	{
		auto& texture = textures[i];

		if (texture.IsValid())
		{
			TransitionBarrier(texture, D3D12_RESOURCE_STATE_RENDER_TARGET);
			renderTargetDescriptors.push_back(texture.GetRTV());

			TrackResource(texture);
		}
	}

	// Bind Depth texture
	const auto& depthTexture = renderTarget.GetTexture(AttachmentPoint::DepthStencil);

	CD3DX12_CPU_DESCRIPTOR_HANDLE depthStencilDescriptor(D3D12_DEFAULT);
	if (depthTexture.GetD3D12Resource())
	{
		TransitionBarrier(depthTexture, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		depthStencilDescriptor = depthTexture.GetDSV();

		TrackResource(depthTexture);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* dsv = depthStencilDescriptor.ptr != 0 ? &depthStencilDescriptor : nullptr;

	m_d3d12CommandList->OMSetRenderTargets(static_cast<UINT>(renderTargetDescriptors.size()), renderTargetDescriptors.data(), FALSE, dsv);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
	FlushResourceBarriers();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	m_d3d12CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void CommandList::Dispatch(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
{
	FlushResourceBarriers();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i]->CommitStagedDescriptorsForDispatch(*this);
	}

	m_d3d12CommandList->Dispatch(numGroupsX, numGroupsY, numGroupsZ);
}

bool CommandList::Close(CommandList& pendingCommandList)
{
	// Flush any remaining barriers
	FlushResourceBarriers();

	m_d3d12CommandList->Close();

	// Flush pending resource barriers
	uint32_t numPendingBarriers = m_resourceStateTracker->FlushPendingResourceBarriers(pendingCommandList);
	// Commit the final resource state to the global state
	m_resourceStateTracker->CommitFinalResourceStates();

	return numPendingBarriers > 0;
}

void CommandList::Close()
{
	FlushResourceBarriers();
	m_d3d12CommandList->Close();
}

void CommandList::Reset()
{
	ThrowIfFailed(m_d3d12CommandAllocator->Reset());
	ThrowIfFailed(m_d3d12CommandList->Reset(m_d3d12CommandAllocator.Get(), nullptr));

	m_resourceStateTracker->Reset();
	m_uploadBuffer->Reset();

	ReleaseTrackedObjects();

	for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_dynamicDescriptorHeap[i]->Reset();
		m_descriptorHeaps[i] = nullptr;
	}

	m_rootSignature = nullptr;
	m_computeCommandList = nullptr;
}

void CommandList::TrackResource(const Resource& res)
{
	TrackResource(res.GetD3D12Resource());
}

void CommandList::TrackResource(Microsoft::WRL::ComPtr<ID3D12Object> object)
{
	m_trackedObjects.push_back(object);
}

void CommandList::ReleaseTrackedObjects()
{
	m_trackedObjects.clear();
}

void CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap)
{
	if (m_descriptorHeaps[heapType] != heap)
	{
		m_descriptorHeaps[heapType] = heap;
		BindDescriptorHeaps();
	}
}

void CommandList::LoadFromTextureFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage)
{
	fs::path filePath(fileName);

	if (!fs::exists(filePath))
	{
		throw std::exception("File not found!");
	}

	std::lock_guard<std::mutex> lock(s_textureCacheMutex);
	auto it = s_textureCache.find(fileName);
	if (it != s_textureCache.end())
	{
		texture.SetTextureUsage(textureUsage);
		texture.SetD3D12Resource(it->second);
		texture.CreateViews();
		texture.SetName(fileName);
	}
	else
	{
		TexMetadata metadata;
		ScratchImage scratchImage;

		if (filePath.extension() == ".dds")
		{
			ThrowIfFailed(
				LoadFromDDSFile(fileName.c_str(), DDS_FLAGS_FORCE_RGB, &metadata, scratchImage)
			);
		}
		else if (filePath.extension() == ".hdr")
		{
			ThrowIfFailed(
				LoadFromHDRFile(fileName.c_str(), &metadata, scratchImage)
			);
		}
		else if (filePath.extension() == ".tga")
		{
			ThrowIfFailed(
				LoadFromTGAFile(fileName.c_str(), &metadata, scratchImage)
			);
		}
		else
		{
			ThrowIfFailed(
				LoadFromWICFile(fileName.c_str(), WIC_FLAGS_FORCE_RGB, &metadata, scratchImage)
			);
		}

		//if (textureUsage == TextureUsage::Albedo)
		//{
		//	metadata.format = MakeSRGB(metadata.format);
		//}

		D3D12_RESOURCE_DESC textureDesc = {};
		switch (metadata.dimension)
		{
		case TEX_DIMENSION_TEXTURE1D:
			textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT16>(metadata.arraySize));
			break;
		case TEX_DIMENSION_TEXTURE2D:
			textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>( metadata.height), static_cast<UINT16>(metadata.arraySize));
			break;
		case TEX_DIMENSION_TEXTURE3D:
			textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>(metadata.height), static_cast<UINT16>(metadata.depth));
			break;
		default:
			throw std::exception("Invalid texture dimension!");
			break;
		}

		auto device = Application::Get().GetDevice();
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&textureResource)
		));

		texture.SetTextureUsage(textureUsage);
		texture.SetD3D12Resource(textureResource);
		texture.CreateViews();
		texture.SetName(fileName);

		// Update the global state tracker
		ResourceStateTracker::AddGlobalResourceState(textureResource.Get(), D3D12_RESOURCE_STATE_COMMON);

		std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
		const Image* images = scratchImage.GetImages();
		for (int i = 0; i < scratchImage.GetImageCount(); ++i)
		{
			auto& subresource = subresources[i];
			subresource.RowPitch = images[i].rowPitch;
			subresource.SlicePitch = images[i].slicePitch;
			subresource.pData = images[i].pixels;
		}

		CopyTextureSubresource(texture, 0, static_cast<uint32_t>(subresources.size()), subresources.data());

		if (subresources.size() < textureResource->GetDesc().MipLevels)
		{
			GenerateMips(texture);
		}

		// Add texture to the cache
		s_textureCache[fileName] = textureResource.Get();
	}
}

void CommandList::GenerateMips(Texture& texture)
{
	if (m_d3d12CommandListType == D3D12_COMMAND_LIST_TYPE_COPY)
	{
		if (!m_computeCommandList)
		{
			m_computeCommandList = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE)->GetCommandList();
		}
		m_computeCommandList->GenerateMips(texture);
		return;
	}

	auto resource = texture.GetD3D12Resource();

	// If the texture doesn't have a valid resource, do nothing
	if (!resource)
	{
		return;
	}

	auto resourceDesc = resource->GetDesc();
	// Do nothing if texture has only 1 level (mip 0)
	if (resourceDesc.MipLevels == 1)
	{
		return;
	}

	// Currently, only non-multi-sampled 2D textures are supported
	if (resourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
		resourceDesc.DepthOrArraySize != 1 ||
		resourceDesc.SampleDesc.Count > 1)
	{
		throw std::exception("GenerateMips is only supported for non-multi-sampled 2D Textures!");
	}

	ComPtr<ID3D12Resource> uavResource = resource;
	// Create an alias of the original resource
	// This is done to perform a GPU copy of resources with different formats
	// BGR -> RGB texture copies will fail GPU validation unless performed 
	// through an alias of the BRG resource in a placed heap
	ComPtr<ID3D12Resource> aliasResource;

	if (!texture.CheckUAVSupport() ||
		(resourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
	{
		auto device = Application::Get().GetDevice();

		// Create alias desc for aliased resource
		auto aliasDesc = resourceDesc;

		// Placed resources can't be render targets or depth-stencil views
		aliasDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		aliasDesc.Flags &= ~(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		// Describe a UAV compatible resource that is used to perform mipmapping of the original texture
		auto uavDesc = aliasDesc;
		uavDesc.Format = Texture::GetUAVCompatibleFormat(resourceDesc.Format);

		D3D12_RESOURCE_DESC resourceDescs[] = {
			aliasDesc,
			uavDesc
		};

		// Create a heap that is large enough to store a copy of the original resource
		auto allocationInfo = device->GetResourceAllocationInfo(0, _countof(resourceDescs), resourceDescs);
		D3D12_HEAP_DESC heapDesc = {};
		heapDesc.SizeInBytes = allocationInfo.SizeInBytes;
		heapDesc.Alignment = allocationInfo.Alignment;
		heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
		heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;

		ComPtr<ID3D12Heap> heap;
		ThrowIfFailed(device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)));

		TrackResource(heap);

		// Create a placed resource that matches the description of the original resource
		// This resource is used to copy the original texture to the UAV compatible resource
		ThrowIfFailed(device->CreatePlacedResource(heap.Get(), 0, &aliasDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&aliasResource)));

		ResourceStateTracker::AddGlobalResourceState(aliasResource.Get(), D3D12_RESOURCE_STATE_COMMON);

		// Ensure the scope of the alias resource
		TrackResource(aliasResource);

		// Create a UAV compatible resource in the same heap as the alias resource
		ThrowIfFailed(device->CreatePlacedResource(heap.Get(), 0, &uavDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&uavResource)));

		ResourceStateTracker::AddGlobalResourceState(uavResource.Get(), D3D12_RESOURCE_STATE_COMMON);

		// Ensure the scope of the UAV compatible resource
		TrackResource(uavResource);

		// Copy original resource to alias resource
		AliasingBarrier(nullptr, aliasResource);

		// Copy the original resource to the alias resource
		// This ensures GPU validation
		CopyResource(aliasResource, resource);

		// Add an aliasing barrier for the UAV compatible resource
		AliasingBarrier(aliasResource, uavResource);
	}

	// Generate mips with the UAV compatible resource
	auto tex = Texture(uavResource, texture.GetTextureUsage());
	GenerateMips_UAV(tex, resourceDesc.Format);

	if (aliasResource)
	{
		AliasingBarrier(uavResource, aliasResource);

		// Copy the alias resource back to the original resource
		CopyResource(resource, aliasResource);
	}
}

void CommandList::GenerateMips_UAV(Texture& texture, DXGI_FORMAT format)
{
	if (!m_generateMipsPSO)
	{
		m_generateMipsPSO = std::make_unique<GenerateMipsPSO>();
	}

	m_d3d12CommandList->SetPipelineState(m_generateMipsPSO->GetPipelineState().Get());
	SetComputeRootSignature(m_generateMipsPSO->GetRootSignature());

	GenerateMipsCB generateMipsCB;
	generateMipsCB.IsSRGB = Texture::IsSRGBFormat(format);

	auto resource = texture.GetD3D12Resource();
	auto resourceDesc = resource->GetDesc();

	// Create an SRV that uses the format of the original texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;  // Only 2D textures are supported (this was checked in the calling function)
	srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;

	for (uint32_t srcMip = 0; srcMip < resourceDesc.MipLevels - 1u;)
	{
		uint64_t srcWidth = resourceDesc.Width >> srcMip;
		uint32_t srcHeight = resourceDesc.Height >> srcMip;
		uint32_t dstWidth = static_cast<uint32_t>(srcWidth >> 1);
		uint32_t dstHeight = srcHeight >> 1;

		// 0b00(0): Both width and height are even
		// 0b01(1): Width is odd, height is even
		// 0b10(2): Width is even, height is odd
		// 0b11(3): Both width and height are odd
		generateMipsCB.SrcDimension = (srcHeight & 1) << 1 | (srcWidth & 1);

		// How many mipmap levels to compute this pass (max 4 mips per pass)
		DWORD mipCount;

		// The number of times we can half the size of the texture and get exactly a 50% reduction in size
		// A 1 bit in the width or height indicates an odd dimension
		// The case where either the width or the height is exactly 1 is handled as a special case (as the dimension does not require reduction)
		_BitScanForward(&mipCount, (dstWidth == 1 ? dstHeight : dstWidth) | (dstHeight == 1 ? dstWidth : dstHeight));

		// Maximum number of mips to generate is 4
		mipCount = std::min<DWORD>(4, mipCount + 1);

		// Clamp to total nuber of mips left
		mipCount = (srcMip + mipCount) >= resourceDesc.MipLevels ? resourceDesc.MipLevels - srcMip - 1 : mipCount;

		// Dimensions should not reduce to 0
		// This can happen if the width and height are not the same
		dstWidth = std::max<DWORD>(1, dstWidth);
		dstHeight = std::max<DWORD>(1, dstHeight);

		generateMipsCB.SrcMipLevel = srcMip;
		generateMipsCB.NumMipLevels = mipCount;
		generateMipsCB.TexelSize.x = 1.0f / (float)dstWidth;
		generateMipsCB.TexelSize.y = 1.0f / (float)dstHeight;

		SetCompute32BitConstants(GenerateMips::GenerateMipsCB, generateMipsCB);

		SetShaderResourceView(GenerateMips::SrcMip, 0, texture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, srcMip, 1, &srvDesc);

		for (uint32_t mip = 0; mip < mipCount; ++mip)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = resourceDesc.Format;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = srcMip + mip + 1;

			SetUnorderedAccessView(GenerateMips::OutMip, mip, texture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, srcMip + mip + 1, 1, &uavDesc);
		}

		// Pad any unused mip levels with a default UAV. Doing this keeps the DX12 runtime happy
		if (mipCount < 4)
		{
			m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(GenerateMips::OutMip, mipCount, 4 - mipCount, m_generateMipsPSO->GetDefaultUAV());
		}

		Dispatch(Math::DivideByMultiple(dstWidth, 8), Math::DivideByMultiple(dstHeight, 8));

		UAVBarrier(texture);

		srcMip += mipCount;
	}
}

void CommandList::BindDescriptorHeaps()
{
	UINT numDescriptorHeaps = 0;
	ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		ID3D12DescriptorHeap* descriptorHeap = m_descriptorHeaps[i];
		if (descriptorHeap)
		{
			descriptorHeaps[numDescriptorHeaps++] = descriptorHeap;
		}
	}

	m_d3d12CommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
}