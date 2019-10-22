#pragma once

#include "CommandListManager.h"

class Resource;
class UploadBuffer;
class DynamicDescriptorHeap;
class RootSignature;
class RenderTarget;

class Buffer;
class IndexBuffer;
class VertexBuffer;
class RawBuffer;
class StructuredBuffer;


#include <vector>
#include <queue>

class ContextManager
{
public:
	CommandContext* AllocateContext(D3D12_COMMAND_LIST_TYPE type);
	void FreeContext(CommandContext context);
	void FreeAll();

private:
	std::vector<std::unique_ptr<CommandContext>> m_contextPool[4];
	std::queue<CommandContext*> m_availableContexts[4];
	std::mutex m_contextAllocationMutex;
};

struct NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};

class CommandContext : NonCopyable
{
	friend ContextManager;

public:
	~CommandContext();

	static void DestroyAllContexts();

	static CommandContext& Start(const std::wstring id = L"");

	// Flush and release
	uint64_t Finish(bool WaitForCompletion = false);

	// Flush existing commands to the GPU but keep the context alive
	uint64_t Flush(bool WaitForCompletion = false);

	// Prepare to render by reserving comm list amd comm alloc
	void Initialize();

	RenderContext& GetRenderContext()
	{
		assert(m_type != D3D12_COMMAND_LIST_TYPE_COMPUTE);
		return reinterpret_cast<RenderContext&>(*this);
	}

	ComputeContext& GetComputeContext()
	{
		assert(m_type == D3D12_COMMAND_LIST_TYPE_COMPUTE);
		return reinterpret_cast<ComputeContext&>(*this);
	}

	ID3D12GraphicsCommandList* GetGraphicsCommandList()
	{
		return m_commandList;
	}

	// Texture
	void LoadFromTextureFile(Texture& texture, const std::wstring& fileName);

	// Resources and barriers
	void TransitionResource(Resource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
	void BeginResourceTransition(Resource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
	void InsertUAVBarrier(Resource& Resource, bool FlushImmediate = false);
	void InsertAliasBarrier(Resource& Before, Resource& After, bool FlushImmediate = false);
	inline void FlushResourceBarriers(void);

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, ID3D12DescriptorHeap* heap);

protected:
	// Binds the current descriptor heaps to the command list
	void BindDescriptorHeaps();

	CommandListManager* m_owningManager;
	ID3D12GraphicsCommandList* m_commandList;
	ID3D12CommandAllocator* m_currentAllocator;

	ID3D12RootSignature* m_currentGraphicsRootSignature;
	ID3D12PipelineState* m_currentGraphicsPipelineState;
	ID3D12RootSignature* m_currentComputeRootSignature;
	ID3D12PipelineState* m_currentComputePipelineState;

	// Heap CBV_SRV_UAV
	DynamicDescriptorHeap m_dynamicViewDescriptorHeap;

	//Heap SAMPLER
	DynamicDescriptorHeap m_dynamicSamplerDescriptorHeap;

	D3D12_RESOURCE_BARRIER m_resourceBarrierBuffer[16];
	UINT m_numBarriersToFlush;

	ID3D12DescriptorHeap* m_currentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	D3D12_COMMAND_LIST_TYPE m_type;

private:
	CommandContext(D3D12_COMMAND_LIST_TYPE type);
	void Reset();


	ID3D12GraphicsCommandList* m_commandList;
};

class RenderContext : public CommandContext
{

};


class ComputeContext : public CommandContext
{

};