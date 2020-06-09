#pragma once

#include <Render/Materials/MaterialBase.h>

namespace alexis
{
	class SkyboxMaterial : public MaterialBase
	{
	public:
		SkyboxMaterial();

		virtual void SetupToRender(CommandContext* commandContext) override;
	};

}