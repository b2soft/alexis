#pragma once

#include <Render/Materials/MaterialBase.h>

namespace alexis
{
	class ShadowMaterial : public MaterialBase
	{
	public:
		ShadowMaterial();

		virtual void SetupToRender(CommandContext* commandContext) override;
	};

}