#pragma once
#include "project_dependencies.h" // IWYU pragma: keep

#include <cstdlib>
#include <string>
#include <vector>

// for certain configurations, provide (simplified) implementations of C++11 or newer functionality
#if defined(__cplusplus) && __cplusplus < 201103L && defined(__MINGW32__) && !defined(__clang__)
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

// clang provided libc++ does have those defined mistakenly in C++98 mode
// simplified implementation only capable of parsing one number
inline double stod(const std::wstring& str)
{
    wchar_t* end;
    return std::wcstod(str.c_str(), &end);
}

inline int stoi(const std::wstring& str)
{
    wchar_t* end;
    return std::wcstol(str.c_str(), &end, 10);
}
} // namespace std
#endif

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

inline std::vector<std::wstring> StringSplit(const std::wstring& input, const wchar_t delimiter)
{
    std::vector<std::wstring> result;
    std::wstring              s   = input;
    size_t                    pos = s.find(delimiter);
    while (pos != std::wstring::npos)
    {
        result.push_back(s.substr(0, pos));
        s   = s.substr(pos + 1);
        pos = s.find(delimiter);
    }
    result.push_back(s);
    return result;
}

inline std::wstring ToCamelCase(const std::wstring& input) // only ASCII - not unicode capable!
{
    std::wstring s = input;
#define IS_UPPER_CASE(x) (x) >= L'A' && (x) <= L'Z'
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (IS_UPPER_CASE(s[i]) && (i == 0 || std::isspace(s[i - 1])))
            s[i] = ((wchar_t)((unsigned int)s[i] - 0x20));
    }
    return s;
}

inline std::wstring TrimWhiteSpaces(const std::wstring& input)
{
    std::wstring       s     = input;
    const std::wstring query = L" \t\r\n";
    size_t             pos   = s.find_first_not_of(query);
    // leading
    if (pos != std::wstring::npos)
    {
        s = s.substr(pos);
    }
    // trailing
    pos = s.find_last_not_of(query);
    if (pos != std::wstring::npos)
    {
        s = s.substr(0, pos + 1);
    }
    // alternative:
    // for(size_t i = 0; i < input.size();++i)
    // {
    //     if(!std::isspace(input[i]))
    //     {
    //         s+=input[i];
    //     }
    // }
    return s;
}
