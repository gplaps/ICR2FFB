#pragma once
#include "project_dependencies.h"

#include <algorithm>
#include <cstdlib>
#include <string>

// a function-like macro to check return code like FAILED/SUCCESS for HRESULT
#define STATUS_CHECK(func) \
    res = (func);          \
    if (res) return res

template <typename T>
int sign(T input)
{
    return (input > (T)0) ? 1 : (input < 0 ? -1 : 0);
}

#if defined(__cplusplus) && __cplusplus < 201703L
namespace std
{
template <typename T> T clamp(T v, const T& lo, const T& hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}
} // namespace std
#endif

template <typename T>
T saturate(T v) { return std::clamp(v, static_cast<T>(0), static_cast<T>(1)); }

inline std::wstring ToLower(const std::wstring& str)
{
    std::wstring result = str;
    for (wchar_t& ch : result) ch = towlower(ch);
    return result;
}

inline std::string ToLower(const std::string& str)
{
    std::string result = str;
    for (char& ch : result) ch = static_cast<char>(tolower(ch));
    return result;
}

inline std::wstring AnsiToWide(const char* str)
{
    size_t       len = std::mbstowcs(NULL, str, 0);
    std::wstring asWide(len, L'\0');
    std::mbstowcs(const_cast<wchar_t*>(asWide.data()), str, len + 1);
    return asWide;
}

#if defined(UNICODE)
inline std::wstring ToWideString(const TCHAR* s) { return std::wstring(s); }
#else
inline std::wstring ToWideString(const TCHAR* s) { return AnsiToWide(s); }
#endif
