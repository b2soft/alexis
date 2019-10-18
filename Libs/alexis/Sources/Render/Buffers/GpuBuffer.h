#pragma once

#include <Render/Descriptors/DescriptorAllocation.h>

#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

namespace alexis
{
	class GpuBuffer
	{
	public:
		void Create(std::size_t numElements, std::size_t elementsSize);

		ID3D12Resource* GetResource() const
		{
			return m_resource.Get();
		}

		void SetFromResource(ID3D12Resource* other);

	protected:
		virtual void CreateViews() = 0;

		Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

		std::size_t m_numElements{ 0 };
		std::size_t m_elementsSize{ 0 };
		std::size_t m_bufferSize{ 0 };
	};

	class VertexBuffer : public GpuBuffer
	{
	public:
		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const;

	private:
		virtual void CreateViews() override;

		D3D12_VERTEX_BUFFER_VIEW m_view;
	};

	class ConstantBuffer : public GpuBuffer
	{
	public:
		void Create(std::size_t numElements, std::size_t elementsSize);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCBV() const
		{
			return m_cbv.GetDescriptorHandle();
		}

	private:
		virtual void CreateViews() override;

		DescriptorAllocation m_cbv;
	};

	class TextureBuffer :public GpuBuffer
	{
	public:
		void Create(uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t numMips = 1);

		D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const
		{
			return m_srv.GetDescriptorHandle();
		}

	private:
		virtual void CreateViews() override;

		DescriptorAllocation m_srv;
	};

	class DynamicConstantBuffer
	{
	public:

		void Create(std::size_t numElements, std::size_t elementsSize);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCBV() const
		{
			return m_cbv.GetDescriptorHandle();
		}

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUPtr() const
		{
			return m_gpuPtr;
		}

		void* GetCPUPtr() const
		{
			return m_cpuPtr;
		}


	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

		std::size_t m_numElements{ 0 };
		std::size_t m_elementsSize{ 0 };
		std::size_t m_bufferSize{ 0 };

		D3D12_GPU_VIRTUAL_ADDRESS m_gpuPtr;
		void* m_cpuPtr{ nullptr };

		DescriptorAllocation m_cbv;
	};
}