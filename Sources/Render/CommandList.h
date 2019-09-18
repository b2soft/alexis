#pragma once

#include <d3d12.h>

#include <wrl.h>

#include <map>
#include <memory.h>
#include <mutex>
#include <vector>

#include "Texture.h"

class Resource;
class UploadBuffer;
class DynamicDescriptorHeap;
class ResourceStateTracker;
class RootSignature;
class RenderTarget;
class GenerateMipsPSO;

class Buffer;
class IndexBuffer;
class VertexBuffer;
class RawBuffer;
class StructuredBuffer;

class CommandList
{
public:
	CommandList(D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandList();

	D3D12_COMMAND_LIST_TYPE GetCommandListType() const
	{
		return m_d3d12CommandListType;
	}

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList() const
	{
		return m_d3d12CommandList;
	}

	void TransitionBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
	void TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);
	
	void AliasingBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> srcResource, Microsoft::WRL::ComPtr<ID3D12Resource> dstResource, bool flushBarriers = false);
	void AliasingBarrier(const Resource& srcResource, const Resource& dstResource, bool flushBarriers = false);

	void UAVBarrier(Microsoft::WRL::ComPtr<ID3D12Resource> resource, bool flushBarriers = false);
	void UAVBarrier(const Resource& resource, bool flushBarriers = false);

	void FlushResourceBarriers();

	void CopyResource(Microsoft::WRL::ComPtr<ID3D12Resource> dstRes, Microsoft::WRL::ComPtr<ID3D12Resource> srcRes);
	void CopyResource(Resource& dstRes, const Resource& srcRes);

	void ResolveSubresource(Resource& dstRes, const Resource& srcRes, uint32_t dstSubresource = 0, uint32_t srcSubresource = 0);

	// Buffer copy
	void CopyBuffer(Buffer& buffer, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void CopyVertexBuffer(VertexBuffer& vertexBuffer, size_t numVertices, size_t vertexStride, const void* vertexBufferData);
	template<class T>
	void CopyVertexBuffer(VertexBuffer& vertexBuffer, const std::vector<T>& vertexBufferData)
	{
		CopyVertexBuffer(vertexBuffer, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
	}

	void CopyIndexBuffer(IndexBuffer& indexBuffer, size_t numIndices, DXGI_FORMAT indexFormat, const void* indexBufferData);
	template<class T>
	void CopyIndexBuffer(IndexBuffer& indexBuffer, const std::vector<T>& indexBufferData)
	{
		assert(sizeof(T) == 2 || sizeof(T) == 4);
		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

		CopyIndexBuffer(indexBuffer, indexBufferData.size(), indexFormat, indexBufferData.data());
	}

	void CopyRawBuffer(RawBuffer& rawBuffer, size_t bufferSize, const void* bufferData);
	template<class T>
	void CopyRawBuffer(RawBuffer& rawBuffer, const T& data)
	{
		CopyRawBuffer(rawBuffer, sizeof(T), &data);
	}

	void CopyStructuredBuffer(StructuredBuffer& structuredBuffer, size_t numElements, size_t elementSize, const void* bufferData);
	template<class T>
	void CopyStructuredBuffer(StructuredBuffer& structuredBuffer, const std::vector<T>& bufferData)
	{
		CopyStructuredBuffer(structuredBuffer, bufferData.size(), sizeof(T), bufferData.data());
	}

	void SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);

	void SetViewport(const D3D12_VIEWPORT& viewport);
	void SetViewports(const std::vector<D3D12_VIEWPORT>& viewports);

	void SetScissorRect(const D3D12_RECT& scissorRect);
	void SetScissorRects(const std::vector<D3D12_RECT>& scissorRects);

	void SetPipelineState(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState);

	void SetGraphicsRootSignature(const RootSignature& rootSignature);
	void SetComputeRootSignature(const RootSignature& rootSignature);

	void ClearTexture(const Texture& texture, const float clearColor[4]);
	void ClearDepthStencilTexture(const Texture& texture, D3D12_CLEAR_FLAGS clearFlags, float depth = 1.0f, uint8_t stencil = 0);

	void CopyTextureSubresource(Texture& texture, uint32_t firstSubresource, uint32_t numSubresources, D3D12_SUBRESOURCE_DATA* subresourceData);

	void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
	template<class T>
	void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, const T& data)
	{
		SetGraphicsDynamicConstantBuffer(rootParameterIndex, sizeof(T), &data);
	}

	void SetGraphics32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
	template<class T>
	void SetGraphics32BitConstants(uint32_t rootParameterIndex, const T& constants)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be 4 bytes multiple!");
		SetGraphics32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
	}

	void SetCompute32BitConstants(uint32_t rootParameterIndex, uint32_t numConstants, const void* constants);
	template<class T>
	void SetCompute32BitConstants(uint32_t rootParameterIndex, const T& constants)
	{
		static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be 4 bytes multiple!");
		SetCompute32BitConstants(rootParameterIndex, sizeof(T) / sizeof(uint32_t), &constants);
	}

	void SetVertexBuffer(uint32_t slot, const VertexBuffer& vertexBuffer);
	void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData);
	template<class T>
	void SetDynamicVertexBuffer(uint32_t slot, const std::vector<T>& vertexBufferData)
	{
		SetDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
	}

	void SetIndexBuffer(const IndexBuffer& indexBuffer);
	void SetDynamicIndexBuffer(size_t numIndicies, DXGI_FORMAT indexFormat, const void* indexBufferData);
	template<class T>
	void SetDynamicIndexBuffer(const std::vector<T>& indexBufferData)
	{
		static_assert(sizeof(T) == 2 || sizeof(T) == 4, "Size of type must be 16 or 32 bit multiple!");

		DXGI_FORMAT indexFormat = (sizeof(T) == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		SetDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.data());
	}

	void SetGraphicsDynamicStructuredBuffer(uint32_t slot, size_t numElements, size_t elementSize, const void* bufferData);
	template<class T>
	void SetGraphicsDynamicStructuredBuffer(uint32_t slot, const std::vector<T>& bufferData)
	{
		SetGraphicsDynamicStructuredBuffer(slot, bufferData.size(), sizeof(T), bufferData.data());
	}

	void SetShaderResourceView(
		uint32_t rootParameterIndex,
		uint32_t descriptorOffset,
		const Resource& resource,
		D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
		UINT firstSubresource = 0,
		UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		const D3D12_SHADER_RESOURCE_VIEW_DESC * srv = nullptr
	);

	void SetUnorderedAccessView(
		uint32_t rootParameterIndex,
		uint32_t descrptorOffset,
		const Resource& resource,
		D3D12_RESOURCE_STATES stateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		UINT firstSubresource = 0,
		UINT numSubresources = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* uav = nullptr);

	void SetRenderTarget(const RenderTarget& renderTarget);

	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);
	void Dispatch(uint32_t numGroupsX, uint32_t numGroupsY = 1, uint32_t numGroupsZ = 1);

	bool Close(CommandList& pendingCommandList);
	void Close();

	void Reset();

	void ReleaseTrackedObjects();

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

	void LoadFromTextureFile(Texture& texture, const std::wstring& fileName, TextureUsage textureUsage);

	void GenerateMips(Texture& texture);

	void GenerateMips_UAV(Texture& texture, DXGI_FORMAT format);

private:
	void TrackResource(Microsoft::WRL::ComPtr<ID3D12Object> object);
	void TrackResource(const Resource& res);

	// Binds the current descriptor heaps to the command list.
	void BindDescriptorHeaps();

	D3D12_COMMAND_LIST_TYPE m_d3d12CommandListType;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_d3d12CommandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_d3d12CommandAllocator;

	// For copy queues, it may be necessary to generate mips while loading textures.
	// Mips can't be generated on copy queues but must be generated on compute or
	// direct queues. In this case, a Compute command list is generated and executed 
	// after the copy queue is finished uploading the first sub resource.
	std::shared_ptr<CommandList> m_computeCommandList;

	// Keep track of the currently bound root signatures to minimize root
	// signature changes.
	ID3D12RootSignature* m_rootSignature;

	// Resource created in an upload heap. Useful for drawing of dynamic geometry
	// or for uploading constant buffer data that changes every draw call.
	std::unique_ptr<UploadBuffer> m_uploadBuffer;

	// Resource state tracker is used by the command list to track (per command list)
	// the current state of a resource. The resource state tracker also tracks the 
	// global state of a resource in order to minimize resource state transitions.
	std::unique_ptr<ResourceStateTracker> m_resourceStateTracker;

	// The dynamic descriptor heap allows for descriptors to be staged before
	// being committed to the command list. Dynamic descriptors need to be
	// committed before a Draw or Dispatch.
	std::unique_ptr<DynamicDescriptorHeap> m_dynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	// Keep track of the currently bound descriptor heaps. Only change descriptor 
	// heaps if they are different than the currently bound descriptor heaps.
	ID3D12DescriptorHeap* m_descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	std::unique_ptr<GenerateMipsPSO> m_generateMipsPSO;

	// Objects that are being tracked by a command list that is "in-flight" on 
	// the command-queue and cannot be deleted. To ensure objects are not deleted 
	// until the command list is finished executing, a reference to the object
	// is stored. The referenced objects are released when the command list is 
	// reset.
	using TrackedObjects = std::vector<Microsoft::WRL::ComPtr<ID3D12Object>>;
	TrackedObjects m_trackedObjects;

	static std::mutex s_textureCacheMutex;
	static std::map<std::wstring, ID3D12Resource*> s_textureCache;
};