#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define _ENABLE_EXTENDED_ALIGNED_STORAGE

#include <windows.h>
#include <shellapi.h> // For CommandLineToArgvW

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif