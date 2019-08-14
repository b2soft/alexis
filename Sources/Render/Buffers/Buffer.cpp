#include <Precompiled.h>

#include "Buffer.h"

Buffer::Buffer(const std::wstring& name /*= L""*/)
	: Resource(name)
{
}

Buffer::Buffer(const D3D12_RESOURCE_DESC& resDesc, size_t numElements, size_t elemetSize, const std::wstring& name /*= L""*/)
	: Resource(resDesc, nullptr, name)
{
	CreateViews(numElements, elemetSize);
}

void Buffer::CreateViews(size_t numElements, size_t elementSize)
{
	throw std::exception("Unimplemented function!");
}
