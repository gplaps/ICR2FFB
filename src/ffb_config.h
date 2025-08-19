#pragma once

#include "game_version.h"

#include <string>

/* project specific ini parser */
struct FFBConfig
{
    FFBConfig();
    bool        Valid() const;
    GameVersion version;

    // private:
    // Search the ini file for settings and find what the user has set them to
    int LoadSettingsFromConfig();

    //settings from the ffb.ini
    std::wstring targetDeviceName;
    std::wstring targetGameVersion;
    std::wstring targetGameWindowName;
    std::wstring targetForceSetting;
    std::wstring targetDeadzoneSetting;
    std::wstring targetInvertFFB;
    std::wstring targetLimitEnabled;
    std::wstring targetConstantEnabled;
    std::wstring targetConstantScale;
    std::wstring targetBrakingScale;
    std::wstring targetWeightEnabled;
    std::wstring targetWeightScale;
    std::wstring targetDamperEnabled;
    std::wstring targetDamperScale;
    std::wstring targetSpringEnabled;

private:
    bool         LoadFFBSettings(const std::wstring& filename);
    GameVersion  ReadGameVersion(const std::wstring& versionText);
    std::wstring PrintGameVersion() const;
};
