#include "ffb_config.h"

#include "helpers.h"
#include "log.h"

#include <fstream>
#include <iostream>

/*
 * Copyright 2025 gplaps
 *
 * Licensed under the MIT License (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/MIT
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */

FFBConfig::FFBConfig() :
    version()
{
    if (!LoadSettingsFromConfig())
    {
        return; // yes, useless statement as nothing follows
    }
}

bool FFBConfig::Valid() const { return version != VERSION_UNINITIALIZED; }

// Search the ini file for settings and find what the user has set them to
bool FFBConfig::LoadFFBSettings(const std::wstring& filename)
{
    // safety ONLY through master force scale!
    // scale and percent ... lets be cautious ... misleading naming
    // targetConstantScale = L"100.0";
    // targetDamperScale = L"100.0";
    targetWeightScale = L"100.0";

    std::wifstream file(filename.c_str());
    if (!file) { return false; }
    std::wstring line;
    while (std::getline(file, line))
    {
        if (line.rfind(L"Device: ", 0) == 0)
        {
            targetDeviceName = line.substr(8);
        }
        else if (line.rfind(L"Game: ", 0) == 0)
        {
            targetGameWindowName = line.substr(6);
        }
        else if (line.rfind(L"Version: ", 0) == 0)
        {
            targetGameVersion = line.substr(9);
        }
        else if (line.rfind(L"Force: ", 0) == 0)
        {
            targetForceSetting = line.substr(7);
        }
        else if (line.rfind(L"Deadzone: ", 0) == 0)
        {
            targetDeadzoneSetting = line.substr(10);
        }
        else if (line.rfind(L"Invert: ", 0) == 0)
        {
            targetInvertFFB = line.substr(8);
        }
        else if (line.rfind(L"Limit: ", 0) == 0)
        {
            targetLimitEnabled = line.substr(7);
        }
        else if (line.rfind(L"Constant: ", 0) == 0)
        {
            targetConstantEnabled = line.substr(10);
        }
        else if (line.rfind(L"Constant Scale: ", 0) == 0)
        {
            targetConstantScale = line.substr(16);
        }
        else if (line.rfind(L"Weight: ", 0) == 0)
        {
            targetWeightEnabled = line.substr(8);
        }
        else if (line.rfind(L"Weight Scale: ", 0) == 0)
        {
            targetWeightScale = line.substr(14);
        }
        else if (line.rfind(L"Damper: ", 0) == 0)
        {
            targetDamperEnabled = line.substr(8);
        }
        else if (line.rfind(L"Damper Scale: ", 0) == 0)
        {
            targetDamperScale = line.substr(14);
        }
        else if (line.rfind(L"Spring: ", 0) == 0)
        {
            targetSpringEnabled = line.substr(8);
        }
    }
    return !targetDeviceName.empty();
}

int FFBConfig::LoadSettingsFromConfig()
{
    // Load FFB configuration file "ffb.ini"
    if (!LoadFFBSettings(L"ffb.ini"))
    {
        LogMessage(L"[ERROR] Failed to load FFB settings from ffb.ini");
        LogMessage(L"[ERROR] Make sure ffb.ini exists in current working directory and has proper format");

        // SHOW ERROR ON CONSOLE immediately
        std::wcout << L"[ERROR] Failed to load FFB settings from ffb.ini" << L'\n';
        std::wcout << L"[ERROR] Make sure ffb.ini exists in current working directory and has proper format" << L'\n';
        std::wcout << L"Press any key to exit..." << L'\n';
        std::cin.get();
        return 1;
    }

    LogMessage(L"[INFO] Successfully loaded FFB settings");
    LogMessage(L"[INFO] Target device: " + targetDeviceName);

    version = ReadGameVersion(targetGameVersion);
    LogMessage(L"[INFO] Game version: " + PrintGameVersion());
#if !defined(NDEBUG)
// just in case there is a difference in the threading API
#    if defined(HAS_STL_THREAD_MUTEX)
    LogMessage(L"[Debug] Thread with STL");
#    else
    LogMessage(L"[Debug] Thread with Windows API");
#    endif
#endif
    return 0;
}

GameVersion FFBConfig::ReadGameVersion(const std::wstring& versionText)
{
    // Select between Dos and Rendition version. Rendition is default
    return ToLower(versionText) == L"dos4g" ? ICR2_DOS4G_1_02 : ICR2_RENDITION;
}

std::wstring FFBConfig::PrintGameVersion() const
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
