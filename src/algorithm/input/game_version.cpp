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

Game::Game(BaseGame game, Renderer renderer, VersionInfo version, BinaryType BinaryType, const GameOffsets& offsets) :
    mProduct(game),
    mRenderer(renderer),
    mVersion(version),
    mBinaryInfo(BinaryType),
    mOffsets(offsets) {}

Game::Game() :
    mProduct(),
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

static std::wstring BinaryInfoToString(BinaryType options)
{
    switch (options)
    {
        case DOS32A:                 return L"DOS32/A";
        case DOS4GW:                 return L"DOS/4G";
        case WIN32_APPLICATION:      return L"WIN32";
        case UNDETECTED_BINARY_TYPE:
        default:                     break;
    }
    return L"Unspecified";
}

std::wstring Game::ToString() const
{
    std::wstringstream ss;
    ss << BaseGameToString(mProduct) << L" - ";
    ss << RendererToString(mRenderer) << L" - ";
    ss << VersionToString(mVersion) << L" - ";
    ss << BinaryInfoToString(mBinaryInfo);
    return ss.str();
}

bool Game::Valid() const
{
    return mProduct != UNDETECTED_GAME &&
        mRenderer != UNDETECTED_RENDERER &&
        mBinaryInfo != UNDETECTED_BINARY_TYPE &&
        mVersion != UNDETECTED_VERSION;
}

const GameOffsets& Game::Offsets() const
{
    return mOffsets;
}

BaseGame Game::Product() const
{
    return mProduct;
}

void Game::ApplySignature(uintptr_t signatureAddress)
{
    mOffsets.ApplySignature(signatureAddress);
}

bool Game::IsThis(BaseGame game, Renderer renderer, VersionInfo version, BinaryType BinaryType) const
{
    return mProduct == game &&
        mRenderer == renderer &&
        mBinaryInfo == BinaryType &&
        mVersion == version;
}

Game SupportedGames::FindGame(BaseGame game, Renderer renderer, VersionInfo version, BinaryType BinaryType)
{
    for (size_t i = 0; i < gameList.size(); ++i)
    {
        const Game& entry = gameList[i];
        if (entry.IsThis(game, renderer, version, BinaryType))
        {
            return entry; // with valid GameOffsets
        }
    }
    GameOffsets invalidOffsets = {};
    return Game(game, renderer, version, BinaryType, invalidOffsets);
}
