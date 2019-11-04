#pragma once

#include <array>
#include <map>
#include <mutex>

#include <Render/RootSignature.h>
#include <Render/Buffers/GpuBuffer.h>
#include <Render/RenderTarget.h>
#include <Render/Descriptors/DynamicDescriptorHeap.h>

namespace alexis
{
	class CommandContext
	{
	public:
		CommandContext();
		//CommandContext(const CommandContext&) = delete;
		//
		//CommandContext(CommandContext&& other) :
		//	Allocator(std::move(other.Allocator)),
		//	List(std::move(other.List)),
		//	DynamicDescriptors(std::move(other.DynamicDescriptors)),
		//	DescriptorHeap{ other.DescriptorHeap },
		//	m_rootSignature{ other.m_rootSignature }
		//{
		//}

		~CommandContext()
		{
			//__debugbreak();
		}

		void DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation);
		void DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount = 1, UINT startIndexLocation = 0, INT baseVertexLocation = 0, UINT startInstanceLocation = 0);

		void SetRootSignature(const RootSignature& rootSignature);

		void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

		void SetSRV(uint32_t rootParameterIdx, uint32_t descriptorOffset, const TextureBuffer& resource);
		void SetCBV(uint32_t rootParameterIdx, uint32_t descriptorOffset, const ConstantBuffer& resource);
		void SetDynamicCBV(uint32_t rootParameterIdx, std::size_t bufferSize, const void* bufferData);

		void ClearTexture(const TextureBuffer& texture, const float clearColor[4]);
		void ClearDepthStencil(const TextureBuffer& texture, D3D12_CLEAR_FLAGS clearFlags, float depth = 1.0f, uint8_t stencil = 0);

		void SetRenderTarget(const RenderTarget& renderTarget);
		void SetViewport(const D3D12_VIEWPORT& viewport);
		void SetViewports(const std::vector<D3D12_VIEWPORT>& viewports);

		void TransitionResource(const GpuBuffer& resource, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState);

		void CopyBuffer(GpuBuffer& destination, const void* data, std::size_t numElements, std::size_t elementSize);

		void InitializeTexture(TextureBuffer& destination, UINT numSubresources, D3D12_SUBRESOURCE_DATA subData[]);

		void LoadTextureFromFile(TextureBuffer& destination, const std::wstring& filename);

		void BindDescriptorHeaps();

		void Reset();

		ComPtr<ID3D12CommandAllocator> Allocator;
		ComPtr<ID3D12GraphicsCommandList> List;
		std::array<DynamicDescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> DynamicDescriptors;
		std::array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> DescriptorHeap;

	private:
		ID3D12RootSignature* m_rootSignature{ nullptr };

		static std::mutex s_textureCacheMutex;
		static std::map<std::wstring, ID3D12Resource*> s_textureCache;
	};
}
