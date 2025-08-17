#pragma once

// === Windows ===
#if !defined(NOMINMAX)
#    define NOMINMAX
#endif
#if !defined(WIN32_LEAN_AND_MEAN)
#    define WIN32_LEAN_AND_MEAN
#endif
#include <initguid.h>
#include <windows.h>

// compiler and C++ standard differences
#if defined(__cplusplus) && __cplusplus >= 201103L && !(defined(_MSC_VER) && _MSC_VER <= 1800)
#    define IS_CPP11_COMPLIANT
#endif

// C++98 does not contain threads and mutexes, so use Windows API instead
#if defined(__cplusplus) && __cplusplus >= 201103L
#    define HAS_STL_THREAD_MUTEX
#endif
