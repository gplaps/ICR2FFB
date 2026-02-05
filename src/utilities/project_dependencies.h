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

#if defined(__cplusplus) && __cplusplus >= 201103L
#    define NO_EXCEPT noexcept
#    define OVERRIDE  override
#else
#    define NO_EXCEPT
#    define OVERRIDE
#endif

// C++98 does not contain threads and mutexes, so use Windows API instead
#if defined(__cplusplus) && __cplusplus >= 201103L
#    define HAS_STL_THREAD_MUTEX
#endif

#if defined(HAS_STL_THREAD_MUTEX)
#    include <mutex> // IWYU pragma: keep
#    define LOCK_MUTEX(mtx) \
        const std::lock_guard<std::mutex> lock(mtx)
#    define TRY_LOCK_MUTEX(mtx) \
        mtx.try_lock()
#    define UNLOCK_MUTEX(mtx)
#    define DEFINE_MUTEX(mtx) \
        std::mutex mtx
#    define DECLARE_MUTEX(mtx) \
        std::mutex mtx
#else
#    include <synchapi.h>
#    define LOCK_MUTEX(mtx) \
        EnterCriticalSection(mtx)
#    define TRY_LOCK_MUTEX(mtx) \
        TryEnterCriticalSection(mtx)
#    define UNLOCK_MUTEX(mtx) \
        LeaveCriticalSection(mtx)
#    define DEFINE_MUTEX(mtx) \
        LPCRITICAL_SECTION mtx = NULL
#    define DECLARE_MUTEX(mtx) \
        LPCRITICAL_SECTION mtx
#endif
