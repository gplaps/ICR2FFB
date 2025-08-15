#include "wstring_helpers.h"

std::wstring ToLower(const std::wstring &str)
{
    std::wstring result = str;
    for (wchar_t &ch : result)
        ch = towlower(ch);
    return result;
}
