#pragma once

#include <vector>

#include "TransformComponent.h"
#include "ModelComponent.h"

namespace alexis
{
	namespace ecs
	{
		class ModelSystem
		{
		public:
			void Draw();

			void AddModel(std:unique_ptr<ModelComponent> model);
			void RemoveAllModels();

		private:
			std::vector<std:unique_ptr<ModelComponent>> m_models;
		};
	}
}