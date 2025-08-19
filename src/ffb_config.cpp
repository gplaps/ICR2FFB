#include "ffb_config.h"

#include "helpers.h"
#include "log.h"
#include "version.h"

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

void FFBConfig::RegisterSettings()
{
    settings.push_back(Setting(L"General", L"Device", L"FFB Wheel", L"list your device here with the exact name it uses in the game controllers menu\nyou can also use the device index (1, 2, 3, 4 etc) instead"));
    settings.push_back(Setting(L"General", L"Game", L"indycar", L"list the exe name you use ie. 'indycar' or 'cart'"));
    settings.push_back(Setting(L"General", L"Version", L"DOS4G", L"which version of the game you are trying to run 'REND32A' or 'OS4G'"));
    settings.push_back(Setting(L"General", L"Force", 25.0, L"Master toggle for what \\% of force do you want? 1 - 100"));
    settings.push_back(Setting(L"General", L"Deadzone", 0.0, L"add a deadzone (based on lateral G) where forces do not apply (0 - 100)"));
    settings.push_back(Setting(L"General", L"Invert", false, L"changes the direction of the ffb. Change this to 'true' if your wheel pulls in the wrong direction"));
    settings.push_back(Setting(L"General", L"Limit", false, L"this limits the effect refresh rate to be more compatible with older or Belt-drive wheels\ngive it a try if you get really abrupt forces or no force at all"));
    settings.push_back(Setting(L"Effects", L"Constant", true, L"General main effect which will add force to the wheel to simulate G load, this is the main force"));
    settings.push_back(Setting(L"Effects", L"Constant Scale", 100.0, L"General main effect which will add force to the wheel to simulate G load, this is the main force"));
    settings.push_back(Setting(L"Effects", L"Braking Scale", 100.0, L"How much Braking/Accelerating effects forces. Tune this if you want to feel the brake pedal more or less (can go above 100%)."));
    settings.push_back(Setting(L"Effects", L"Damper", true, L"Damper adds friction to the wheel at slow speed (under 50mph) to help simulate non-powered steering"));
    settings.push_back(Setting(L"Effects", L"Damper Scale", 50.0, L"Damper adds friction to the wheel at slow speed (under 50mph) to help simulate non-powered steering"));
    settings.push_back(Setting(L"Effects", L"Spring", false, L"Spring adds a centering force to the wheel unrelated to physics\nI recommend keeping this off unless you just like the wheel to center itself not based on physics"));
}

bool FFBConfig::GetBool(const std::wstring& key) const
{
    for (size_t i = 0; i < settings.size(); ++i)
    {
        const Setting& setting = settings[i];
        if (setting.mValue.mType == ST_BOOL && ToLower(setting.mKey) == key)
        {
            return setting.mValue.b;
        }
    }
    return false;
}

std::wstring FFBConfig::GetString(const std::wstring& key) const
{
    for (size_t i = 0; i < settings.size(); ++i)
    {
        const Setting& setting = settings[i];
        if (setting.mValue.mType == ST_STRING && ToLower(setting.mKey) == key)
        {
            return setting.mValue.s;
        }
    }
    return L"";
}

double FFBConfig::GetDouble(const std::wstring& key) const
{
    for (size_t i = 0; i < settings.size(); ++i)
    {
        const Setting& setting = settings[i];
        if (setting.mValue.mType == ST_DOUBLE && ToLower(setting.mKey) == key)
        {
            return setting.mValue.d;
        }
    }
    return 0.0;
}

bool FFBConfig::ParseLine(const std::wstring& line)
{
    if (line.size() && line[0] == L'#') { return true; } // skip commented line
    for (size_t i = 0; i < settings.size(); ++i)
    {
        Setting& setting = settings[i];
        if (line.rfind(setting.mKey) == 0 && line.find(L':') != std::wstring::npos)
        {
            const std::wstring valueString = line.substr(line.find(L':') + 1);
            switch (setting.mValue.mType)
            {
                case ST_BOOL:
                    setting.mValue.b = ToLower(valueString) == L"true";
                    break;
                case ST_STRING:
                    setting.mValue.s = valueString;
                    break;
                case ST_DOUBLE:
                    setting.mValue.d = std::stod(valueString);
                    break;
                default: break;
            }
            return true;
        }
    }
    LogMessage(L"[INFO] Ignoring ini line: \"" + line + L'\"');
    return false;
}

void FFBConfig::WriteIniFile()
{
    std::wofstream file(L"ffb.ini");
    if (!file)
    {
        LogMessage(L"[ERROR] Failed to write ffb.ini");
    }
    else
    {
        for (size_t i = 0; i < settings.size(); ++i)
        {
            const Setting& setting = settings[i];
            file << setting.mKey << L": ";
            switch (setting.mValue.mType)
            {
                case ST_BOOL:
                    file << (setting.mValue.b ? L"true" : L"false");
                    break;
                case ST_STRING:
                    file << setting.mValue.s;
                    break;
                case ST_DOUBLE:
                    file << std::to_wstring(setting.mValue.d);
                    break;
                default: break;
            }
            if (!setting.mDescription.empty())
            {
                file << L" #" << setting.mDescription;
            }
            file << L'\n';
        }
    }
}

// Search the ini file for settings and find what the user has set them to
bool FFBConfig::LoadFFBSettings(const std::wstring& filename)
{
    std::wifstream file(filename.c_str());
    if (!file)
    {
        LogMessage(L"[INFO] No ini file found, creating default log.txt");
        WriteIniFile();
        return false;
    }
    std::wstring line;
    while (std::getline(file, line))
    {
        ParseLine(line);
    }
    return !GetString(L"device").empty();
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
    LogMessage(L"[INFO] Target device: " + GetString(L"device"));

    version = ReadGameVersion(GetString(L"version"));
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

FFBConfig::Setting::Setting(const std::wstring& section, const std::wstring& key, bool defaultValue, const std::wstring& description) :
    mSection(section),
    mKey(key),
    mDescription(description),
    mValue(defaultValue) {}

FFBConfig::Setting::Setting(const std::wstring& section, const std::wstring& key, const wchar_t* defaultValue, const std::wstring& description) :
    mSection(section),
    mKey(key),
    mDescription(description),
    mValue(defaultValue) {}

FFBConfig::Setting::Setting(const std::wstring& section, const std::wstring& key, double defaultValue, const std::wstring& description) :
    mSection(section),
    mKey(key),
    mDescription(description),
    mValue(defaultValue) {}
