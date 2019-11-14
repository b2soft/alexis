#pragma once

#include <Render/Materials/MaterialBase.h>

namespace alexis
{
	class Hdr2SdrMaterial : public MaterialBase
	{
	public:
		Hdr2SdrMaterial();

		virtual void SetupToRender(CommandContext* commandContext) override;
	};

}