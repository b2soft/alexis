#pragma once

#include <ECS/ECS.h>

namespace alexis
{
	namespace ecs
	{
		class EditorSystem : public System
		{
		public:
			void Update(float dt);
		};

	}
}