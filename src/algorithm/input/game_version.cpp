#include "game_version.h"

#include <sstream>

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

SupportedGame::SupportedGame(BaseGame game, Renderer renderer, VersionInfo version, BinaryOptions binaryOptions, GameOffsets offsets) :
    mGame(game),
    mRenderer(renderer),
    mVersion(version),
    mBinaryInfo(binaryOptions),
    mOffsets(offsets) {}

SupportedGame::SupportedGame() :
    mGame(),
    mRenderer(),
    mVersion(),
    mBinaryInfo(),
    mOffsets() {}

static std::wstring BaseGameToString(BaseGame game)
{
    switch (game)
    {
        case INDYCAR_RACING_2: return L"INDYCAR Racing 2";
        case NASCAR_RACING_1:  return L"NASCAR Racing 1";
        case NASCAR_RACING_2:  return L"NASCAR Racing 2";
        case UNDETECTED_GAME:
        default:               break;
    }
    return L"Unspecified";
}

static std::wstring RendererToString(Renderer renderer)
{
    switch (renderer)
    {
        case SOFTWARE:            return L"Software";
        case RENDITION:           return L"Rendition";
        case UNDETECTED_RENDERER:
        default:                  break;
    }
    return L"Unspecified";
}

static std::wstring VersionToString(VersionInfo version)
{
    switch (version)
    {
        case V1_0_0:             return L"1.0.0";
        case V1_0_2:             return L"1.0.2";
        case V1_2_1:             return L"1.21";
        case V2_0_2:             return L"2.0.2";
        case V2_0_3:             return L"2.0.3";
        case UNDETECTED_VERSION:
        default:                 break;
    }
    return L"Unspecified";
}

static std::wstring BinaryInfoToString(BinaryOptions options)
{
    switch (options)
    {
        case DOS32A:                    return L"DOS32/A";
        case DOS4GW:                    return L"DOS/4G";
        case WIN32_APPLICATION:         return L"WIN32";
        case UNDETECTED_BINARY_OPTIONS:
        default:                        break;
    }
    return L"Unspecified";
}

std::wstring SupportedGame::ToString() const
{
    std::wstringstream ss;
    ss << BaseGameToString(mGame) << L" - ";
    ss << RendererToString(mRenderer) << L" - ";
    ss << VersionToString(mVersion) << L" - ";
    ss << BinaryInfoToString(mBinaryInfo);
    return ss.str();
}

bool SupportedGame::Valid() const
{
    return mGame != UNDETECTED_GAME &&
        mRenderer != UNDETECTED_RENDERER &&
        mBinaryInfo != UNDETECTED_BINARY_OPTIONS &&
        mVersion != UNDETECTED_VERSION;
}

const GameOffsets& SupportedGame::Offsets() const
{
    return mOffsets;
}

BaseGame SupportedGame::Game() const
{
    return mGame;
}

void SupportedGame::ApplySignature(uintptr_t signatureAddress)
{
    mOffsets.ApplySignature(signatureAddress);
}

bool SupportedGame::IsThis(BaseGame game, Renderer renderer, VersionInfo version, BinaryOptions binaryOptions) const
{
    return mGame == game &&
        mRenderer == renderer &&
        mBinaryInfo == binaryOptions &&
        mVersion == version;
}

SupportedGame SupportedGames::FindGame(BaseGame game, Renderer renderer, VersionInfo version, BinaryOptions binaryOptions)
{
    for (size_t i = 0; i < SupportedGames::gameList.size(); ++i)
    {
        const SupportedGame& entry = SupportedGames::gameList[i];
        if (entry.IsThis(game, renderer, version, binaryOptions))
        {
            return entry; // with valid GameOffsets
        }
    }
    return SupportedGame();
}
