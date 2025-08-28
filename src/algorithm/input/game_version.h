#pragma once

#include "project_dependencies.h" // IWYU pragma: keep

#include "log.h"
#include "string_utilities.h"

enum GameVersion
#if defined(IS_CPP11_COMPLIANT)
    : unsigned char
#endif
{
    VERSION_UNINITIALIZED,
    ICR2_DOS4G_1_02,
    ICR2_RENDITION,
    ICR2_WINDOWS,
    NASCAR1,
    NASCAR2_V2_03
};

inline GameVersion ReadGameVersion(const std::wstring& versionText)
{
    // Select game version
    std::wstring gameVersionLower = ToLower(versionText);

    if (gameVersionLower == L"icr2dos") {
        return ICR2_DOS4G_1_02;
    }
    else if (gameVersionLower == L"icr2rend") {
        return ICR2_RENDITION;
    }
    else if (gameVersionLower == L"icr2windy") {
        return ICR2_WINDOWS;
    }
    else if (gameVersionLower == L"nascar1") {
        return NASCAR1;
    }
    else if (gameVersionLower == L"nascar2") {
        return NASCAR2_V2_03;
    }
    else {
        LogMessage(L"[ERROR] Invalid game version: " + versionText);
        return VERSION_UNINITIALIZED;
    }
}

inline std::wstring PrintGameVersion(GameVersion version)
{
    switch (version)
    {
        case ICR2_DOS4G_1_02: return L"ICR2 - Dos4G 1.02";
        case ICR2_RENDITION:  return L"ICR2 - Rendition";
        case ICR2_WINDOWS:    return L"ICR2 - Windows";
        case NASCAR1:         return L"NASCAR1";
        case NASCAR2_V2_03:   return L"NASCAR2 - V2.03";
        case VERSION_UNINITIALIZED:
        default:
            break;
    }
    return L"Uninitialized";
}
