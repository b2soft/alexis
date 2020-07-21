#pragma once

#include <DirectXMath.h>

namespace alexis
{
	namespace ecs
	{
		struct LightComponent
		{
			enum class LightType
			{
				Point = 0,
				Spot
			};

			LightType Type{ LightType::Point };
			DirectX::XMVECTOR Position;
			DirectX::XMVECTOR Color;
		};
	}
}
