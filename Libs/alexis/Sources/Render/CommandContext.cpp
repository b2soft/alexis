#include <Precompiled.h>

#include "CommandContext.h"

#include <Core/Render.h>
#include <Render/Buffers/UploadBufferManager.h>

namespace alexis
{

	CommandContext::CommandContext()
	{

	}

	void CommandContext::DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation)
	{
		for (auto& descriptor : DynamicDescriptors)
		{
			descriptor->CommitStagedDescriptorsForDraw(this);
		}

		List->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
	}

	void CommandContext::SetRootSignature(const RootSignature& rootSignature)
	{
		auto resource = rootSignature.GetRootSignature().Get();

		if (m_rootSignature != resource)
		{
			m_rootSignature = resource;

			for (auto& descriptor : DynamicDescriptors)
			{
				descriptor->ParseRootSignature(rootSignature);
			}

			List->SetGraphicsRootSignature(m_rootSignature);
		}
	}

	void CommandContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap)
	{
		if (DescriptorHeap[heapType] != heap)
		{
			DescriptorHeap[heapType] = heap;
			BindDescriptorHeaps();
		}
	}

	void CommandContext::SetSRV(uint32_t rootParameterIdx, uint32_t descriptorOffset, ColorBuffer& resource)
	{
		DynamicDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIdx, descriptorOffset, 1, resource.GetSRV());
	}

	void CommandContext::SetCBV(uint32_t rootParameterIdx, uint32_t descriptorOffset, ConstantBuffer& resource)
	{
 		DynamicDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIdx, descriptorOffset, 1, resource.GetCBV());
	}

	void CommandContext::TransitionResource(GpuBuffer& resource, D3D12_RESOURCE_STATES newState, bool flushImmediately /*= false*/, D3D12_RESOURCE_STATES oldState /*= D3D12_RESOURCE_STATE_COMMON*/)
	{
		D3D12_RESOURCE_BARRIER& BarrierDesc = m_resourceBarrierBuffer[m_numBarriersToFlush++];

		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Transition.pResource = resource.GetResource();
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = oldState;
		BarrierDesc.Transition.StateAfter = newState;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		//else if (NewState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
			//	InsertUAVBarrier(Resource, FlushImmediate);

		if (flushImmediately || m_numBarriersToFlush == 16)
		{
			FlushResourceBarriers();
		}
	}

	void CommandContext::CopyBuffer(const void* data, std::size_t numElements, std::size_t elementSize, GpuBuffer& destination)
	{
		auto device = Render::GetInstance()->GetDevice();
		auto bufferManager = Render::GetInstance()->GetUploadBufferManager();
		const std::size_t sizeInBytes = numElements * elementSize;

		auto allocation = bufferManager->Allocate(sizeInBytes, elementSize);
		memcpy(allocation.Cpu, data, sizeInBytes);

		List->CopyBufferRegion(destination.GetResource(), 0, allocation.Resource, 0, sizeInBytes);
	}

	void CommandContext::BindDescriptorHeaps()
	{
		UINT numDescriptorHeaps = 0;
		ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

		for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			ID3D12DescriptorHeap* descriptorHeap = DescriptorHeap[i];
			if (descriptorHeap)
			{
				descriptorHeaps[numDescriptorHeaps++] = descriptorHeap;
			}
		}

		List->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
	}

	void CommandContext::Reset()
	{
		Allocator->Reset();
		List->Reset(Allocator.Get(), nullptr);

		for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			DynamicDescriptors[i]->Reset();
			DescriptorHeap[i] = nullptr;
		}

		m_rootSignature = nullptr;
	}

}