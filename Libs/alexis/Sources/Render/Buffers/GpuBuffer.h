#pragma once

#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

namespace alexis
{
	class GpuBuffer
	{
	public:
		GpuBuffer() = default;

		GpuBuffer(const GpuBuffer&) = delete;
		GpuBuffer& operator=(const GpuBuffer&) = delete;

		GpuBuffer(GpuBuffer&&) = default;
		GpuBuffer& operator=(GpuBuffer&&) = default;

		virtual ~GpuBuffer() = default;

		void Create(std::size_t numElements, std::size_t elementsSize, const D3D12_CLEAR_VALUE* clearValue = nullptr);

		ID3D12Resource* GetResource() const
		{
			return m_resource.Get();
		}

		bool IsValid() const
		{
			return (m_resource != nullptr);
		}

		D3D12_RESOURCE_DESC GetResourceDesc() const
		{
			return m_resource->GetDesc();
		}

		void Reset();

		void SetFromResource(ID3D12Resource* other);

	protected:
		virtual void CreateViews() {}

		Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
		std::unique_ptr<D3D12_CLEAR_VALUE> m_clearValue;

		std::size_t m_numElements{ 0 };
		std::size_t m_elementSize{ 0 };
		std::size_t m_bufferSize{ 0 };
	};

	class VertexBuffer : public GpuBuffer
	{
	public:
		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
		{
			return m_view;
		}

	private:
		virtual void CreateViews() override;

		D3D12_VERTEX_BUFFER_VIEW m_view;
	};

	class IndexBuffer : public GpuBuffer
	{
	public:
		D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const
		{
			return m_view;
		}

	private:
		virtual void CreateViews() override;

		D3D12_INDEX_BUFFER_VIEW m_view;
	};

	class TextureBuffer : public GpuBuffer
	{
	public:
		void Create(uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t numMips = 1, const D3D12_CLEAR_VALUE* clearValue = nullptr);
		void Create(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue = nullptr);
		void CreateFromSwapchain(ID3D12Resource* resource);

		void Resize(uint32_t width, uint32_t height, uint32_t depthOrArraySize = 1);
	};
}