#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _ENABLE_EXTENDED_ALIGNED_STORAGE

#include <windows.h>
#include <shellapi.h> // For CommandLineToArgvW

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXTex.h>

// D3D12 extension library.
#include "d3dx12.h"

// STL Headers
#include <algorithm>
#include <cassert>
#include <chrono>
#include <memory>

#include <array>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

#include "Helpers.h"

namespace fs = std::filesystem;
using namespace DirectX;