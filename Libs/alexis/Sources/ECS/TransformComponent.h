#pragma once

#include <DirectXMath.h>

namespace alexis
{
	namespace ecs
	{
		class TransformComponent
		{
		public:
			//void InitFromJson( const std::wstring& json);

			XMMATRIX GetTransform();
			XMVECTOR GetPosition();
			XMVECTOR GetRotation();

		private:
			XMVECTOR m_position;
			XMVECTOR m_rotation; // quaternion
			XMMATRIX m_modelMatrix;
		};
	}
}