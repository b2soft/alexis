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
				Directional = 0, // Analytical direction only
				Point, // Analytical coord only
				Omni, // Sphere
				Spot // Cone
			};

			LightType Type{ LightType::Directional };
			DirectX::XMVECTOR Direction;
			DirectX::XMVECTOR Color;
		};
	}
}
