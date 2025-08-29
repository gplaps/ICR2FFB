#include "game_version.h"

// Rendition EXE
static const GameOffsets Offsets_ICR2_REND = {
    0xB1C0C, //signature

    0xE0EA4, //cars data

    0xBB4E8, //lf tire load?
    0xBB4EA, //rf tire load?
    0xBB4E4, //lr tire load?
    0xBB4E6, //rr tire load?

    0xEAB24, //lf tire lat load
    0xEAB26, //rf tire lat load
    0xEAB20, //lr tire lat load
    0xEAB22, //rr tire lat load

    0xEAB04, //lf tire long load
    0xEAB06, //rf tire long load
    0xEAB00, //lr tire long load
    0xEAB02, //rr tire long load

    ICR2SIG_ALL_VERSIONS //offset base
};

// DOS4G Exe, should be 1.02
static const GameOffsets Offsets_ICR2_DOS = {
    0xA0D78,

    0xD4718,

    0xA85B8,
    0xA85BA,
    0xA85B4,
    0xA85B6,

    0xC5C48,
    0xC5C4A,
    0xC5C44,
    0xC5C46,

    0xC5C18,
    0xC5C1A,
    0xC5C14,
    0xC5C16,

    ICR2SIG_ALL_VERSIONS};

// ICR2 Windy
static const GameOffsets Offsets_ICR2_WINDY = {
    0x004E2161,

    0x004E0000, /* ???? */ // missing cars data

    0x004F3854,
    0x004F3856,
    0x004F3850,
    0x004F3852,

    0x00528204,
    0x00528206,
    0x00528200,
    0x00528202,

    0x005281F8,
    0x005281Fa,
    0x005281F4,
    0x005281F6,

    ICR2SIG_ALL_VERSIONS};

// N1 Offsets
static const GameOffsets Offsets_NASCAR = {
    0xAEA8C,

    0xEFED4,

    0xCEF70,
    0xCEF70,
    0xCEF70,
    0xCEF70,

    0x9F6F8,
    0x9F6FA,
    0x9f780,
    0x9F6F6,

    0xF0970,
    0xF0970,
    0xF0970,
    0xF0970,

    NR1SIG};

// N2 Offsets
static const GameOffsets Offsets_NASCAR2 = {
    0xD7125, // "NASCAR V2.03"

    0xAD440,

    0xF39FA,
    0xF39FC,
    0xF39F6,
    0xF39F8,

    0xF3B0E,
    0xF3B10,
    0xF3B0A,
    0xF3B0C,

    0xF3B02,
    0xF3B04,
    0xF3AFE,
    0xF3B00,

    NR2SIG};

// Not found
static const GameOffsets Offsets_Unspecified = {
    0x0,

    0x0,

    0x0,
    0x0,
    0x0,
    0x0,

    0x0,
    0x0,
    0x0,
    0x0,

    0x0,
    0x0,
    0x0,
    0x0,

    UNINIT_SIG};

// static std::vector<GameVersion,GameOffsets> GetKnownGameOffsets()
// {
//     std::vector<std::pair<GameVersion,GameOffsets>> result;
//     result.push_back(std::pair<GameVersion,GameOffsets>(ICR2_DOS, Offsets_ICR2_DOS));
//     result.push_back(std::pair<GameVersion,GameOffsets>(ICR2_RENDITION, Offsets_ICR2_REND));
//     result.push_back(std::pair<GameVersion,GameOffsets>(ICR2_WINDOWS, Offsets_ICR2_WINDY));
//     result.push_back(std::pair<GameVersion,GameOffsets>(NASCAR1, Offsets_NASCAR));
//     result.push_back(std::pair<GameVersion,GameOffsets>(NASCAR2, Offsets_NASCAR2));
//     result.push_back(std::pair<GameVersion,GameOffsets>(AUTO_DETECT, Offsets_Unspecified));
//     result.push_back(std::pair<GameVersion,GameOffsets>(VERSION_UNINITIALIZED, Offsets_Unspecified));
//     return result;
// }

GameOffsets GetGameOffsets(GameVersion version)
{
    switch (version)
    {
        case ICR2_DOS:
            return Offsets_ICR2_DOS;
        case ICR2_RENDITION:
            return Offsets_ICR2_REND;
        case ICR2_WINDOWS:
            return Offsets_ICR2_WINDY;
        case NASCAR1:
            return Offsets_NASCAR;
        case NASCAR2:
            return Offsets_NASCAR2;
        case AUTO_DETECT:
        case VERSION_UNINITIALIZED:
        default:
            return Offsets_Unspecified;
    }
}

void GameOffsets::ApplySignature(uintptr_t signatureAddress)
{
    const uintptr_t exeBase = signatureAddress - signature;
    signature               = exeBase;

    cars_data += exeBase;

    tire_data_fl += exeBase;
    tire_data_fr += exeBase;
    tire_data_lr += exeBase;
    tire_data_rr += exeBase;

    tire_maglat_fl += exeBase;
    tire_maglat_fr += exeBase;
    tire_maglat_lr += exeBase;
    tire_maglat_rr += exeBase;

    tire_maglong_fl += exeBase;
    tire_maglong_fr += exeBase;
    tire_maglong_lr += exeBase;
    tire_maglong_rr += exeBase;
}

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
