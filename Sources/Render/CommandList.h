#pragma once

#include <d3d12.h>

#include <wrl.h>

#include <map>
#include <memory.h>
#include <mutex>
#include <vector>

class Resource;
class UploadBuffer;
class DynamicDescriptorHeap;
class ResourceStateTracker;
class RootSignature;
class Texture;
class RenderTarget;

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

	void TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, bool flushBarriers = false);

	void FlushResourceBarriers();

	void CopyResource(Resource& dstRes, const Resource& srcRes);

	void ResolveSubresource(Resource& dstRes, const Resource& srcRes, uint32_t dstSubresource = 0, uint32_t srcSubresource = 0);

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

	void SetGraphicsDynamicConstantBuffer(uint32_t rootParameterIndex, size_t sizeInBytes, const void* bufferData);
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

	void SetRenderTarget(const RenderTarget& renderTarget);

	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);
	void Dispatch(uint32_t numGroupsX, uint32_t numGroupsY = 1, uint32_t numGroupsZ = 1);

	bool Close(CommandList& pendingCommandList);
	void Close();

	void Reset();

	void ReleaseTrackedObjects();

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

private:
	void TrackObject(Microsoft::WRL::ComPtr<ID3D12Object> object);
	void TrackResource(const Resource& res);

	// Binds the current descriptor heaps to the command list.
	void BindDescriptorHeaps();

	using TrackedObjects = std::vector<Microsoft::WRL::ComPtr<ID3D12Object>>;

	D3D12_COMMAND_LIST_TYPE m_d3d12CommandListType;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> m_d3d12CommandList;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_d3d12CommandAllocator;

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

	// Objects that are being tracked by a command list that is "in-flight" on 
	// the command-queue and cannot be deleted. To ensure objects are not deleted 
	// until the command list is finished executing, a reference to the object
	// is stored. The referenced objects are released when the command list is 
	// reset.
	TrackedObjects m_trackedObjects;
};