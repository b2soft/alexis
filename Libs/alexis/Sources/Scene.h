#pragma once

#include <ECS/ECS.h>

// Temp - TODO rework for res manager
#include <Render/Mesh.h>

namespace alexis
{

	class Scene
	{
	public:
		void LoadFromJson(const std::wstring& filename);

	private:
		// TODO move to res manager
		std::vector<std::unique_ptr<Mesh>> m_meshes;
	};
}