#pragma once

#include <Render/Materials/MaterialBase.h>

#include <Render/Buffers/GpuBuffer.h>

namespace alexis
{
	class EnvRectToCubeMaterial : public MaterialBase
	{
	public:
		EnvRectToCubeMaterial();

		virtual void SetupToRender(CommandContext* commandContext) override;

	};
}
