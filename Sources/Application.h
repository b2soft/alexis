#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <memory>
#include <string>

#include "Render/DescriptorAllocation.h"

class Window;
class Game;
class CommandQueue;
class DescriptorAllocator;

class Application
{
public:

	// Create the application singleton with the application instance handle
	static void Create(HINSTANCE hInst);

	// Destroy the app instance and all windows created
	static void Destroy();

	// Get the app singleton
	static Application& Get();

	// Check G-Sync/FreeSync
	bool IsTearingSupported() const;

	// Create DirectX12 render window instance
	std::shared_ptr<Window> CreateRenderWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync = true);

	// Destroy a window by name
	void DestroyWindow(const std::wstring& windowName);

	// Destroy a window by ref
	void DestroyWindow(std::shared_ptr<Window> window);

	// Find a window by name
	std::shared_ptr<Window> GetWindowByName(const std::wstring& windowName);

	// Run the application loop
	int Run(std::shared_ptr<Game> game);

	// Request app to quit
	void Quit(int returnCode = 0);

	// Get D3D12 device
	Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice() const;

	// Get a command queue
	std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;

	// Flush all command queues
	void Flush();

	DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);
	void ReleaseStaleDescriptors(uint64_t finishedFrame);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type);
	UINT GetDescriptorHeapIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

	static uint64_t GetFrameCount()
	{
		return s_frameCount;
	}

protected:
	// Create app instance
	Application(HINSTANCE hInst);

	// Destroy the app instance
	virtual ~Application();

	Microsoft::WRL::ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp) const;
	Microsoft::WRL::ComPtr<ID3D12Device2> CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);
	bool CheckTearingSupport();

private:
	Application(const Application&) = delete;
	Application& operator=(const Application&) = delete;

	// App instance
	HINSTANCE m_hInstance;

	Microsoft::WRL::ComPtr<IDXGIAdapter4> m_dxgiAdapter;
	Microsoft::WRL::ComPtr<ID3D12Device2> m_d3d12Device;

	std::shared_ptr<CommandQueue> m_directCommandQueue;
	std::shared_ptr<CommandQueue> m_computeCommandQueue;
	std::shared_ptr<CommandQueue> m_copyCommandQueue;

	std::unique_ptr<DescriptorAllocator> m_descriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	bool m_tearingSupported{ false };

	static uint64_t s_frameCount;
};