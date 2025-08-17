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

#if defined(HAS_STL_THREAD_MUTEX)
#include <mutex>
#define LOCK_MUTEX(mtx)\
    const std::lock_guard<std::mutex> lock(mtx)
#define UNLOCK_MUTEX(mtx)
#define DEFINE_MUTEX(mtx)\
    std::mutex mtx
#define DECLARE_MUTEX(mtx)\
    extern std::mutex mtx
#else
#include <cassert>
#define LOCK_MUTEX(mtx)\
    assert(WaitForSingleObject(mtx, INFINITE) == WAIT_OBJECT_0)
#define UNLOCK_MUTEX(mtx)\
    ReleaseMutex(mtx)
#define DEFINE_MUTEX(mtx)\
    HANDLE mtx
#define DECLARE_MUTEX(mtx)\
    extern HANDLE mtx
#endif
