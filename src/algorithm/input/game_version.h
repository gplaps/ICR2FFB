#pragma once

#include "project_dependencies.h" // IWYU pragma: keep

#if defined(IS_CPP11_COMPLIANT)
#    include <cstdint>
#else
#    include <stdint.h>
#endif
#include <map>
#include <string>
#include <vector>

// Things to look for in the Memory to make it tick
struct GameOffsets
{
    GameOffsets();
    GameOffsets(const GameOffsets& rhs);
    GameOffsets(uintptr_t sig, uintptr_t car, uintptr_t td_fl, uintptr_t td_fr, uintptr_t td_lr, uintptr_t td_rr, uintptr_t td_mlat_fl, uintptr_t td_mlat_fr, uintptr_t td_mlat_lr, uintptr_t td_mlat_rr, uintptr_t td_mlon_fl, uintptr_t td_mlon_fr, uintptr_t td_mlon_lr, uintptr_t td_mlon_rr);

    GameOffsets& operator=(const GameOffsets& rhs);

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

    void ApplySignature(uintptr_t signatureAddress);

    bool operator==(const GameOffsets& rhs) const;
    bool operator!=(const GameOffsets& rhs) const;
};

enum BaseGame
#if defined(IS_CPP11_COMPLIANT)
    : unsigned char
#endif
{
    UNDETECTED_GAME,
    INDYCAR_RACING_2,
    NASCAR_RACING_1,
    NASCAR_RACING_2
};

enum Renderer
#if defined(IS_CPP11_COMPLIANT)
    : unsigned char
#endif
{
    UNDETECTED_RENDERER,
    SOFTWARE,
    RENDITION
};

enum BinaryType
#if defined(IS_CPP11_COMPLIANT)
    : unsigned char
#endif
{
    UNDETECTED_BINARY_TYPE,
    DOS4GW,
    DOS32A,
    WIN32_APPLICATION
};

enum VersionInfo
#if defined(IS_CPP11_COMPLIANT)
    : unsigned char
#endif
{
    UNDETECTED_VERSION,
    V1_0_0, // ICR2
    V1_0_2, // ICR2
    V1_2_1, // NR1
    V2_0_2, // NR2
    V2_0_3  // NR2
};

struct Game
{
    Game();
    Game(BaseGame game, Renderer renderer, VersionInfo version, BinaryType BinaryType, const GameOffsets& offsets);

    bool               Valid() const;
    std::wstring       ToString() const;
    const GameOffsets& Offsets() const;
    void               ApplySignature(uintptr_t signatureAddress);
    bool               IsThis(BaseGame game, Renderer renderer, VersionInfo version, BinaryType BinaryType) const; // don't like to express with operator== as GameOffsets are not known when looking up entries in the list of known games
    BaseGame           Product() const;

private:
    BaseGame    mProduct;
    Renderer    mRenderer;
    VersionInfo mVersion;
    BinaryType  mBinaryInfo;

    GameOffsets mOffsets;
};

void InitGameDetection();
struct SupportedGames
{
    // Lists of text to look for in the binary
    static std::map<std::string, BaseGame>    baseGameStrings;
    static std::map<std::string, Renderer>    rendererStrings;
    static std::map<std::string, BinaryType>  binaryStrings;
    static std::map<std::string, VersionInfo> versionStrings;

    // Game list
    static Game              FindGame(BaseGame game, Renderer renderer, VersionInfo version, BinaryType BinaryType);
    static std::vector<Game> gameList;
};
