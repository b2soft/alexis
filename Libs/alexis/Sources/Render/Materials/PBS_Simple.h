#pragma once

#include <DirectXMath.h>
#include <Render/Materials/MaterialBase.h>

#include <Render/Buffers/GpuBuffer.h>

namespace alexis
{
	struct PBSMaterialParams
	{
		TextureBuffer* BaseColor;
		float Metallic;
		float Roughness;
	};

	class PBSSimple : public MaterialBase
	{
	public:
		PBSSimple(const PBSMaterialParams& params);

		virtual void SetupToRender(CommandContext* commandContext) override;

	private:
		TextureBuffer* m_baseColor{ nullptr };
		DirectX::XMVECTOR m_metallRoughness;
	};
}
