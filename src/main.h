#pragma once
#include "project_dependencies.h" // IWYU pragma: keep
#if defined(HAS_STL_THREAD_MUTEX)
#    include <atomic>
extern std::atomic<bool> shouldExit;
#else
extern bool shouldExit;
#endif
