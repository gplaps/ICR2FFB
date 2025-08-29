#pragma once

#include "project_dependencies.h" // IWYU pragma: keep

#include "log.h"
#include "string_utilities.h"

// Offsets database for different games/versions
// THANK YOU ERIC

// Provides standardized 'point' to reference for memory
// Maybe this can be replaced with something else more reliable and something that stays the same no matter the game version?
inline const char* ICR2SIG_ALL_VERSIONS = "license with Bob";
inline const char* ICR2SIG_REND         = "Use Rendition"; // or search for "-gRN1f" command line switch
inline const char* ICR2SIG_WINDY        = "<Insert text that only is found in the Windows version of ICR2";
inline const char* NR1SIG               = "name of Harry Gant";
inline const char* NR2SIG               = "NASCAR V2.03"; // too specific - this and some of the game detection mechanism has to be changed if more binaries and their offsets are known
inline const char* UNINIT_SIG           = "TEXT_THAT_SHOULD_NOT_BE_IN_ANY_BINARY_N0Txt2BFouND";

enum GameVersion
#if defined(IS_CPP11_COMPLIANT)
    : unsigned char
#endif
{
    VERSION_UNINITIALIZED,
    ICR2_DOS,
    ICR2_RENDITION,
    ICR2_WINDOWS,
    NASCAR1,
    NASCAR2,

    // detect any supported game - if this does not work, try explicitly requesting a specific game,
    // as the mechanism to detect the games is rudimentary and for example cannot handle multiple Papy games in a DosBox process
    AUTO_DETECT
};

// Things to look for in the Memory to make it tick
struct GameOffsets
{
    uintptr_t signature;

    uintptr_t cars_data;

    uintptr_t tire_data_fl;
    uintptr_t tire_data_fr;
    uintptr_t tire_data_lr;
    uintptr_t tire_data_rr;

    uintptr_t tire_maglat_fl;
    uintptr_t tire_maglat_fr;
    uintptr_t tire_maglat_lr;
    uintptr_t tire_maglat_rr;

    uintptr_t tire_maglong_fl;
    uintptr_t tire_maglong_fr;
    uintptr_t tire_maglong_lr;
    uintptr_t tire_maglong_rr;

    const char* signatureStr;

    void ApplySignature(uintptr_t signatureAddress);
};

GameOffsets GetGameOffsets(GameVersion version);

struct RequestedGame
{
    RequestedGame() {}
    RequestedGame(const std::wstring& versionText);

    std::wstring ToString() const;
    GameVersion  version;
    GameOffsets  offsets;
};
