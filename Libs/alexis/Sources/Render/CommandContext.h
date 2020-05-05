#pragma once

#include <array>
#include <map>
#include <mutex>

#include <Render/Viewport.h>
#include <Render/RootSignature.h>
#include <Render/Buffers/GpuBuffer.h>
#include <Render/RenderTarget.h>
#include <Render/Descriptors/DynamicDescriptorHeap.h>

namespace alexis
{
	class CommandContext
	{
	public:
		explicit CommandContext(D3D12_COMMAND_LIST_TYPE type);

		D3D12_COMMAND_LIST_TYPE GetType() const
		{
			return m_type;
		}

		// Flush commands and keep context alive
		uint64_t Flush(bool waitForCompletion = false);

		// Flush commands and release context
		uint64_t Finish(bool waitForCompletion = false);

		void DrawInstanced(UINT vertexCountPerInstance, UINT instanceCount, UINT startVertexLocation, UINT startInstanceLocation);
		void DrawIndexedInstanced(UINT indexCountPerInstance, UINT instanceCount = 1, UINT startIndexLocation = 0, INT baseVertexLocation = 0, UINT startInstanceLocation = 0);

		void SetRootSignature(const RootSignature& rootSignature);
		// TODO: Do I need wrapper over pipeline state like RootSignature?
		void SetPipelineState(ID3D12PipelineState* pipelineState);

		void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

		void SetSRV(uint32_t rootParameterIdx, uint32_t descriptorOffset, const TextureBuffer& resource);
		void SetCBV(uint32_t rootParameterIdx, uint32_t descriptorOffset, const ConstantBuffer& resource);
		void SetDynamicCBV(uint32_t rootParameterIdx, std::size_t bufferSize, const void* bufferData);

		void ClearTexture(const TextureBuffer& texture, const float clearColor[4]);
		void ClearDepthStencil(const TextureBuffer& texture, D3D12_CLEAR_FLAGS clearFlags, float depth = 1.0f, uint8_t stencil = 0);

		void SetRenderTarget(const RenderTarget& renderTarget);
		void SetViewport(const Viewport& viewport);
		void SetViewports(const std::vector<Viewport>& viewports);

		void TransitionResource(const GpuBuffer& resource, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState);

		void CopyBuffer(GpuBuffer& destination, const void* data, std::size_t numElements, std::size_t elementSize);

		void InitializeTexture(TextureBuffer& destination, UINT numSubresources, D3D12_SUBRESOURCE_DATA subData[]);

		void LoadTextureFromFile(TextureBuffer& destination, const std::wstring& filename);

		void BindDescriptorHeaps();

		void Reset();

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> Allocator;
		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> List;
		std::array<DynamicDescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> DynamicDescriptors;
		std::array<ID3D12DescriptorHeap*, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> DescriptorHeap;

	private:
		ID3D12RootSignature* m_rootSignature{ nullptr };
		D3D12_COMMAND_LIST_TYPE m_type{ D3D12_COMMAND_LIST_TYPE_DIRECT };

		static std::mutex s_textureCacheMutex;
		static std::map<std::wstring, ID3D12Resource*> s_textureCache;
	};
}
