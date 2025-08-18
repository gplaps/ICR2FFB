#pragma once
#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x501 // 0x500 like in project config results in mingw-std-threads errors, but only in cppcheck, not in project itself, haven't looked why, don't ask
#endif
