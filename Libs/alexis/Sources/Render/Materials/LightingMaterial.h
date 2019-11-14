#pragma once

#include <Render/Materials/MaterialBase.h>

namespace alexis
{
	class LightingMaterial : public MaterialBase
	{
	public:
		LightingMaterial();

		virtual void SetupToRender(CommandContext* commandContext) override;
	};

}