#include "game_version.h"

RequestedGame::RequestedGame(const std::wstring& versionText)
{
    // Select game version
    std::wstring gameVersion = ToLower(versionText);

    if (gameVersion == L"icr2dos")
    {
        version = ICR2_DOS;
    }
    else if (gameVersion == L"icr2rend")
    {
        version = ICR2_RENDITION;
    }
    else if (gameVersion == L"icr2windy")
    {
        version = ICR2_WINDOWS;
    }
    else if (gameVersion == L"nascar1")
    {
        version = NASCAR1;
    }
    else if (gameVersion == L"nascar2")
    {
        version = NASCAR2;
    }
    else if (gameVersion.empty() || gameVersion.find(L"auto") != std::wstring::npos)
    {
        version = AUTO_DETECT;
    }
    else
    {
        LogMessage(L"[ERROR] Invalid game version: " + versionText);
        version = VERSION_UNINITIALIZED;
    }
}

std::wstring RequestedGame::ToString() const
{
    switch (version)
    {
        case ICR2_DOS:       return L"ICR2 - Dos4G 1.02";
        case ICR2_RENDITION: return L"ICR2 - Rendition";
        case ICR2_WINDOWS:   return L"ICR2 - Windows";
        case NASCAR1:        return L"NASCAR1";
        case NASCAR2:        return L"NASCAR2 - V2.03";
        case AUTO_DETECT:    return L"Auto detect";
        case VERSION_UNINITIALIZED:
        default:
            break;
    }
    return L"Uninitialized";
}
