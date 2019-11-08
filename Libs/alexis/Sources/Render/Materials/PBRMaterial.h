#pragma once

#include <Render/Materials/MaterialBase.h>

#include <Render/Buffers/GpuBuffer.h>

namespace alexis
{
	struct PBRMaterialParams
	{
		TextureBuffer* BaseColor;
		TextureBuffer* NormalMap;
		TextureBuffer* MetalRoughness;
	};

	class PBRMaterial : public MaterialBase
	{
	public:
		PBRMaterial(const PBRMaterialParams& params);

		virtual void SetupToRender(CommandContext* commandContext) override;

	private:
		TextureBuffer* m_baseColor{ nullptr };
		TextureBuffer* m_normalMap{ nullptr };
		TextureBuffer* m_metalRoughness{ nullptr };
	};
}
