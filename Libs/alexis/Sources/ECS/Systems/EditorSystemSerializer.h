#pragma once

namespace alexis
{
	static struct EditorSystemSerializer
	{
		static void Load(const std::wstring& filename);
		static void Save(const std::wstring& filename);
	};
}