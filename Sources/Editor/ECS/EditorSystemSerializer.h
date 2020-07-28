#pragma once

#include <string>

namespace editor
{
	static struct EditorSystemSerializer
	{
		static void Load(const std::wstring& filename);
		static void Save(const std::wstring& filename);
	};
}