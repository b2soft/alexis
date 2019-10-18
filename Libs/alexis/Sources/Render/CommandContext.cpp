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

	void CommandContext::DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount, UINT startIndexLocation, INT baseVertexLocation, UINT startInstanceLocation)
	{
		for (auto& descriptor : DynamicDescriptors)
		{
			descriptor->CommitStagedDescriptorsForDraw(this);
		}

		List->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
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

	void CommandContext::SetSRV(uint32_t rootParameterIdx, uint32_t descriptorOffset, TextureBuffer& resource)
	{
		DynamicDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIdx, descriptorOffset, 1, resource.GetSRV());
	}

	void CommandContext::SetCBV(uint32_t rootParameterIdx, uint32_t descriptorOffset, ConstantBuffer& resource)
	{
		DynamicDescriptors[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIdx, descriptorOffset, 1, resource.GetCBV());
	}

	void CommandContext::SetDynamicCBV(uint32_t rootParameterIdx, DynamicConstantBuffer& resource)
	{
		List->SetGraphicsRootConstantBufferView(rootParameterIdx, resource.GetGPUPtr());
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

	void CommandContext::CopyBuffer(GpuBuffer& destination, const void* data, std::size_t numElements, std::size_t elementSize)
	{
		auto device = Render::GetInstance()->GetDevice();
		auto bufferManager = Render::GetInstance()->GetUploadBufferManager();
		const std::size_t sizeInBytes = numElements * elementSize;

		auto allocation = bufferManager->Allocate(sizeInBytes, elementSize);
		memcpy(allocation.Cpu, data, sizeInBytes);

		List->CopyBufferRegion(destination.GetResource(), 0, allocation.Resource, allocation.Offset, sizeInBytes);
	}

	void CommandContext::InitializeTexture(TextureBuffer& destination, UINT numSubresources, D3D12_SUBRESOURCE_DATA subData[])
	{
		UINT64 uploadBufferSize = GetRequiredIntermediateSize(destination.GetResource(), 0, numSubresources);

		auto device = Render::GetInstance()->GetDevice();
		auto bufferManager = Render::GetInstance()->GetUploadBufferManager();
		auto allocation = bufferManager->Allocate(uploadBufferSize, 512);

		UpdateSubresources(List.Get(), destination.GetResource(), allocation.Resource, allocation.Offset, 0, numSubresources, subData);
	}

	void CommandContext::LoadTextureFromFile(TextureBuffer& destination, const std::wstring& filename)
	{
		fs::path filePath(filename);

		if (!fs::exists(filePath))
		{
			throw std::exception("File not found!");
		}

		std::lock_guard<std::mutex> lock(s_textureCacheMutex);
		auto it = s_textureCache.find(filename);
		if (it != s_textureCache.end())
		{
			destination.SetFromResource(it->second);
		}
		else
		{
			TexMetadata metadata;
			ScratchImage scratchImage;

			if (filePath.extension() == ".dds")
			{
				ThrowIfFailed(
					LoadFromDDSFile(filename.c_str(), DDS_FLAGS_FORCE_RGB, &metadata, scratchImage)
				);
			}
			else if (filePath.extension() == ".hdr")
			{
				ThrowIfFailed(
					LoadFromHDRFile(filename.c_str(), &metadata, scratchImage)
				);
			}
			else if (filePath.extension() == ".tga")
			{
				ThrowIfFailed(
					LoadFromTGAFile(filename.c_str(), &metadata, scratchImage)
				);
			}
			else
			{
				LoadFromWICFile(filename.c_str(), WIC_FLAGS_FORCE_RGB, &metadata, scratchImage);
			}

			D3D12_RESOURCE_DESC textureDesc = {};
			switch (metadata.dimension)
			{
			case TEX_DIMENSION_TEXTURE1D:
				textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT16>(metadata.arraySize));
				break;
			case TEX_DIMENSION_TEXTURE2D:
				textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>(metadata.height), static_cast<UINT16>(metadata.arraySize));
				break;
			case TEX_DIMENSION_TEXTURE3D:
				textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>(metadata.height), static_cast<UINT16>(metadata.depth));
				break;
			default:
				throw std::exception("Invalid texture dimension!");
				break;
			}

			auto device = Render::GetInstance()->GetDevice();

			std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
			const Image* images = scratchImage.GetImages();
			for (int i = 0; i < scratchImage.GetImageCount(); ++i)
			{
				auto& subresource = subresources[i];
				subresource.RowPitch = images[i].rowPitch;
				subresource.SlicePitch = images[i].slicePitch;
				subresource.pData = images[i].pixels;
			}

			destination.Create(metadata.width, metadata.height, metadata.format, metadata.mipLevels);

			InitializeTexture(destination, subresources.size(), subresources.data());

			// Add texture to the cache
			s_textureCache[filename] = destination.GetResource();
		}
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

	std::mutex CommandContext::s_textureCacheMutex;

	std::map<std::wstring, ID3D12Resource*> CommandContext::s_textureCache;

}