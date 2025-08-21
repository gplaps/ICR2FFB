#pragma once

#include "project_dependencies.h"

#include "string_utilities.h"

enum GameVersion
#if defined(IS_CPP11_COMPLIANT)
    : unsigned char
#endif
{
    VERSION_UNINITIALIZED,
    ICR2_DOS4G_1_02,
    ICR2_RENDITION
};

inline GameVersion ReadGameVersion(const std::wstring& versionText)
{
    // Select between Dos and Rendition version. Rendition is default
    return ToLower(versionText) == L"dos4g" ? ICR2_DOS4G_1_02 : ICR2_RENDITION;
}

inline std::wstring PrintGameVersion(GameVersion version)
{
    switch (version)
    {
        case ICR2_DOS4G_1_02: return L"ICR2 - Dos4G 1.02";
        case ICR2_RENDITION:  return L"ICR2 - Rendition";
        case VERSION_UNINITIALIZED:
        default:
            break;
    }
    return L"Uninitialized";
}
