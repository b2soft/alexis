#pragma once

#include <imgui.h>

#include "d3dx12.h"
#include <wrl.h>

class CommandList;
class Texture;
class RenderTarget;
class RootSignature;
class Window;

class ImguiWrapper
{
public:
	ImguiWrapper();
	virtual ~ImguiWrapper();

	virtual bool Initialize(std::shared_ptr<Window> window);
	virtual void Destroy();

	virtual void NewFrame();
	virtual void Render(std::shared_ptr<CommandList> commandList, const RenderTarget& renderTarget);

private:
	ImGuiContext* m_imguiContext;
	std::unique_ptr<Texture> m_fontTexture;
	std::unique_ptr<RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	std::shared_ptr<Window> m_window;
};

