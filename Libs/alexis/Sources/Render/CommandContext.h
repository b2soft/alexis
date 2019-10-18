#pragma once

#include <array>
#include <map>
#include <mutex>

#include <Render/RootSignature.h>
#include <Render/Buffers/GpuBuffer.h>
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

		void SetSRV(uint32_t rootParameterIdx, uint32_t descriptorOffset, TextureBuffer& resource);
		void SetCBV(uint32_t rootParameterIdx, uint32_t descriptorOffset, ConstantBuffer& resource);
		void SetDynamicCBV(uint32_t rootParameterIdx, DynamicConstantBuffer& resource);

		void TransitionResource(GpuBuffer& resource, D3D12_RESOURCE_STATES newState, bool flushImmediately = false, D3D12_RESOURCE_STATES oldState = D3D12_RESOURCE_STATE_COMMON);

		void CopyBuffer(GpuBuffer& destination, const void* data, std::size_t numElements, std::size_t elementSize );

		void InitializeTexture(TextureBuffer& destination, UINT numSubresources, D3D12_SUBRESOURCE_DATA subData[]);

		void LoadTextureFromFile(TextureBuffer& destination, const std::wstring& filename);

		void BindDescriptorHeaps();

		void Reset();

		void FlushResourceBarriers();

		ComPtr<ID3D12CommandAllocator> Allocator;
		ComPtr<ID3D12GraphicsCommandList> List;
		std::array<DynamicDescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> DynamicDescriptors;
		std::array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> DescriptorHeap;

	private:
		ID3D12RootSignature* m_rootSignature{ nullptr };
		D3D12_RESOURCE_BARRIER m_resourceBarrierBuffer[16];
		UINT m_numBarriersToFlush{ 0 };

		static std::mutex s_textureCacheMutex;
		static std::map<std::wstring, ID3D12Resource*> s_textureCache;
	};

	inline void CommandContext::FlushResourceBarriers()
	{
		if (m_numBarriersToFlush > 0)
		{
			List->ResourceBarrier(m_numBarriersToFlush, m_resourceBarrierBuffer);
			m_numBarriersToFlush = 0;
		}
	}
}
