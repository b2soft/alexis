#pragma once

namespace alexis
{
	namespace ecs
	{
		struct TransformComponent
		{
			DirectX::XMVECTOR Position;
			DirectX::XMVECTOR Rotation;
			float UniformScale;

			bool IsTransformDirty;
		};
	}
}