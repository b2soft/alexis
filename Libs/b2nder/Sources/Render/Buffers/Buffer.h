#pragma once

#include "../Resource.h"

class Buffer : public Resource
{
public:
	Buffer(const std::wstring& name = L"");
	Buffer(const D3D12_RESOURCE_DESC& resDesc, size_t numElements, size_t elemetSize, const std::wstring& name = L"");

	virtual void CreateViews(size_t numElements, size_t elementSize) = 0;
};