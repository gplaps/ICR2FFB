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
T sign(T input)
{
    return (input > static_cast<T>(0)) ? static_cast<T>(1) : (input < 0 ? static_cast<T>(-1) : static_cast<T>(0));
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

// for certain configurations, provide (simplified) implementations of C++11 or newer functionality
#if defined(__cplusplus) && __cplusplus < 201103L && defined(__MINGW32__)
#    include <stdint.h>

#    include <cwchar>

#    if defined(__clang__)
#        pragma clang diagnostic push
#        pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-libc-call" // C++98 has no alternative and the "swprintf is unsafe" suggestion to replace with snprintf (char instead of wchar) doesn't seem right
#    endif
namespace std
{
inline std::wstring to_wstring(int32_t v)
{
    wchar_t buf[128];
    std::swprintf(buf, 128, L"%i", v);
    return std::wstring(buf);
}

#    if defined(__clang__)
#        pragma clang diagnostic pop
#    endif

#    if !defined(__clang__) // clang provided libc++ does have those defined mistakenly in C++98 mode
// simplified implementation only capable of parsing one number
inline double stod(const std::wstring& str)
{
    wchar_t* end;
    return std::wcstod(str.c_str(), &end);
}
#    endif
} // namespace std
#endif

template <typename T>
T saturate(T v) { return std::clamp(v, static_cast<T>(0), static_cast<T>(1)); }

inline std::wstring ToLower(const std::wstring& str)
{
    std::wstring result = str;
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = towlower(result[i]);
    }
    return result;
}

inline std::string ToLower(const std::string& str)
{
    std::string result = str;
    for (size_t i = 0; i < result.size(); ++i)
    {
        result[i] = static_cast<char>(tolower(result[i]));
    }
    return result;
}

#if defined(UNICODE)
inline std::wstring ToWideString(const TCHAR* s) { return std::wstring(s); }
#else

#    if defined(__cplusplus) && __cplusplus < 201103L
#        include <cstdlib>
#    endif

inline std::wstring AnsiToWide(const char* str)
{
#    if defined(__cplusplus) && __cplusplus < 201103L
    size_t       len = mbstowcs(NULL, str, 0);
    std::wstring asWide(len, L'\0');
    mbstowcs(const_cast<wchar_t*>(asWide.data()), str, len + 1);
    return asWide;
#    else
    size_t       len = std::mbstowcs(NULL, str, 0);
    std::wstring asWide(len, L'\0');
    std::mbstowcs(const_cast<wchar_t*>(asWide.data()), str, len + 1);
    return asWide;
#    endif
}

inline std::wstring ToWideString(const TCHAR* s) { return AnsiToWide(s); }
#endif
