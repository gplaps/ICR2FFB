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
    NASCAR2_V2_03,

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

struct DetectedGame
{
    DetectedGame(const std::wstring& versionText);

    std::wstring ToString() const;
    GameVersion version;
    GameOffsets offsets;
}
