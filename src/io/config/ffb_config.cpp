#include "ffb_config.h"

#include "log.h"
#include "string_utilities.h"
#include "version.h"

#include <cstdio>
#include <cwchar>
#include <fstream>
#include <iostream>
#include <stdexcept>

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
    game(),
    settings()
{
    RegisterSettings();
    if (!LoadFFBSettings())
    {
        return; // yes, useless statement as nothing follows
    }
}

bool FFBConfig::Valid() const { return game.version != VERSION_UNINITIALIZED; }

void FFBConfig::RegisterSettings()
{
    settings[L"none"].push_back(Setting(L"none", false, L"not found"));

    settings[L"base"].push_back(Setting(L"device", L"FFB Wheel", L"Name your device here with the exact name it uses in the game controllers menu\nyou can also use the device index (1, 2, 3, 4 etc) instead"));
    settings[L"base"].push_back(Setting(L"game", L"ICR2DOS", L"Select game you are using or leave blank for auto detection. Recognized values are:\n'ICR2DOS', 'ICR2REND', 'ICR2WINDY', 'NASCAR1', 'NASCAR2', 'AUTO'"));
    settings[L"base"].push_back(Setting(L"verbose", false, L"Verbose logging - e.g. used config and FFB thread timing"));

    settings[L"effects"].push_back(Setting(L"force", 25.0, L"Master toggle for what % of force do you want? [0-100]"));
    settings[L"effects"].push_back(Setting(L"deadzone", 0.0, L"add a deadzone (based on lateral G) where forces do not apply [0-100]"));
    settings[L"effects"].push_back(Setting(L"invert", false, L"changes the direction of the ffb. Change this to 'true' if your wheel pulls in the wrong direction"));
    settings[L"effects"].push_back(Setting(L"limit", false, L"this limits the effect refresh rate to be more compatible with older or Belt-drive wheels\ngive it a try if you get really abrupt forces or no force at all"));
    settings[L"effects"].push_back(Setting(L"constant", true, L"Constant enabled - General main effect which will add force to the wheel to simulate G load, this is the main force"));
    settings[L"effects"].push_back(Setting(L"constant scale", 100.0, L"Constant effect strength in % [0-100]"));
    settings[L"effects"].push_back(Setting(L"braking scale", 50.0, L"How much Braking/Accelerating effects forces. Tune this if you want to feel the brake pedal more or less (can go above 100%)."));
    settings[L"effects"].push_back(Setting(L"damper", true, L"Damper enabled - adds friction to the wheel at slow speed (under 50mph) to help simulate non-powered steering"));
    settings[L"effects"].push_back(Setting(L"damper scale", 50.0, L"Damper strength in % [0-100]"));
    settings[L"effects"].push_back(Setting(L"damper speed", 40.0, L"Damper upper speed threshold (mph) where effect strength is zero"));
    settings[L"effects"].push_back(Setting(L"spring", false, L"Spring enabled - Adds a centering force to the wheel unrelated to physics\nI recommend keeping this off unless you just like the wheel to center itself not based on physics"));
    settings[L"effects"].push_back(Setting(L"spring scale", 65.0, L"Spring strength in % [0-100]"));

    sectionDescription.push_back(std::pair<std::wstring, std::wstring>(L"base", L"=== Force feedback device, general settings and game selection ==="));
    sectionDescription.push_back(std::pair<std::wstring, std::wstring>(L"effects", L"=== Effect Mix ===\nEach effect can be turned on or off with the main toggle ('true' or 'false')\nScale settings will control balance for that given force. I personally tuned it at 100% for all of them"));
}

const FFBConfig::Setting& FFBConfig::GetSetting(const std::wstring& section, const std::wstring& key) const
{
    try
    {
        const std::vector<Setting>& sectionSettings = settings.at(section);
        for (size_t i = 0; i < sectionSettings.size(); ++i)
        {
            const Setting& setting = sectionSettings[i];
            if (ToLower(setting.mKey) == key)
            {
                return setting;
            }
        }
    }
    catch (std::out_of_range&)
    {
        const std::vector<Setting>& notFoundSection = settings.at(L"none");
        LogMessage(L"[ERROR] Ivalid setting request: [" + section + L"] " + key);
        return notFoundSection.front();
    }

    const std::vector<Setting>& notFoundSection = settings.at(L"none");
    LogMessage(L"[ERROR] Ivalid setting request: [" + section + L"] " + key);
    return notFoundSection.front();
}

bool FFBConfig::GetBool(const std::wstring& section, const std::wstring& key) const
{
    const Setting& setting = GetSetting(section, key);
    if (setting.mValue.mType == ST_BOOL)
    {
        return setting.mValue.b;
    }
    // std::wcout << L"Programmer error wrong type for setting: " << setting.ToString() << L")\n";
    return false;
}

std::wstring FFBConfig::GetString(const std::wstring& section, const std::wstring& key) const
{
    const Setting& setting = GetSetting(section, key);
    if (setting.mValue.mType == ST_STRING)
    {
        return setting.mValue.s;
    }
    // std::wcout << L"Programmer error wrong type for setting: " << setting.ToString() << L")\n";
    return L"";
}

double FFBConfig::GetDouble(const std::wstring& section, const std::wstring& key) const
{
    const Setting& setting = GetSetting(section, key);
    if (setting.mValue.mType == ST_DOUBLE)
    {
        return setting.mValue.d;
    }
    // std::wcout << L"Programmer error wrong type for setting: " << setting.ToString() << L")\n";
    return 0.0;
}

int FFBConfig::GetInt(const std::wstring& section, const std::wstring& key) const
{
    const Setting& setting = GetSetting(section, key);
    if (setting.mValue.mType == ST_DOUBLE)
    {
        return setting.mValue.i;
    }
    // std::wcout << L"Programmer error wrong type for setting: " << setting.ToString() << L")\n";
    return 0;
}

bool FFBConfig::ParseLine(std::wstring& currentSection, const std::wstring& line)
{
    if (!line.empty() && line[0] == L'#') { return true; } // skip commented line

    // section header []
    const bool isSection = line.find(L'[') == 0 && line.find(L']') != std::wstring::npos;
    if (isSection)
    {
        currentSection = ToLower(line.substr(1, line.find(L']') - 1));
        return true;
    }
    else
    {
        const size_t splitPos = line.find(L'=');
        if (splitPos == std::wstring::npos)
        {
            if (!line.empty())
            {
                LogMessage(L"[WARNING] Skipping malformed ini line: \"" + line + L'\"');
            }
            return false;
        }
        std::vector<Setting>& sectionSettings = settings[currentSection];
        const std::wstring    lineKey         = ToLower(TrimWhiteSpaces(line.substr(0, splitPos)));
        const std::wstring    lineVal         = TrimWhiteSpaces(line.substr(splitPos + 1));
        for (size_t i = 0; i < sectionSettings.size(); ++i)
        {
            Setting& setting = sectionSettings[i];
            if (lineKey == ToLower(setting.mKey))
            {
                const std::wstring valueString = TrimWhiteSpaces(lineVal);
                const bool         valid       = setting.mValue.FromString(valueString);
                if (!valid)
                {
                    LogMessage(L"[WARNING] Parsing value \"" + valueString + L" for \"" + setting.mKey + L"\" failed - default value " + setting.mValue.ToString() + L" used");
                }
                return valid;
            }
        }
    }
    return false;
}

void FFBConfig::WriteFFBIniFile() const
{
    std::wofstream file(L"ffb.ini");
    if (!file)
    {
        LogMessage(L"[ERROR] Failed to write ffb.ini");
    }
    else
    {
        file << L"# " << VERSION_STRING << L" USE AT YOUR OWN RISK\n";
        bool isFirstSecion = true;
        for (std::map<std::wstring, std::vector<Setting> >::const_iterator section = settings.begin(); section != settings.end(); ++section)
        {
            if (section->first == L"none") { continue; }

            if (!isFirstSecion)
            {
                file << L'\n';
            }
            isFirstSecion = false;

            file << L"\n[" << section->first << L"]";
            for (std::vector<std::pair<std::wstring, std::wstring> >::const_iterator it = sectionDescription.begin(); it != sectionDescription.end(); ++it)
            {
                if (it->first == section->first)
                {
                    file << L' ';
                    std::vector<std::wstring> descrLines = StringSplit(it->second, L'\n');
                    for (size_t j = 0; j < descrLines.size(); ++j)
                    {
                        file << L"# " << descrLines[j] << L'\n';
                    }
                    break;
                }
            }
            file << L'\n';

            const std::vector<Setting>& sectionSettings = section->second;
            for (size_t i = 0; i < sectionSettings.size(); ++i)
            {
                const Setting& setting = sectionSettings[i];
                if (i != 0)
                {
                    file << L'\n';
                }
                file << setting.mKey << L" = ";
                file << setting.mValue.ToString() << L'\n';
                if (!setting.mDescription.empty())
                {
                    std::vector<std::wstring> descrLines = StringSplit(setting.mDescription, L'\n');
                    for (size_t j = 0; j < descrLines.size(); ++j)
                    {
                        file << L"# " << descrLines[j] << L'\n';
                    }
                }
            }
        }
    }
}

void FFBConfig::LogConfig() const
{
    LogMessage(L"[INFO] --- Configuration in use ---\n");
    for (std::map<std::wstring, std::vector<Setting> >::const_iterator sectionSettings = settings.begin(); sectionSettings != settings.end(); ++sectionSettings)
    {
        LogMessage(L'[' + ToCamelCase(sectionSettings->first) + L"]");
        for (size_t i = 0; i < sectionSettings->second.size(); ++i)
        {
            const Setting& setting = sectionSettings->second[i];
            LogMessage(setting.ToString());
        }
    }
    LogMessage(L"\n[INFO] --- Configuration end ---");
}

// Search the ini file for settings and find what the user has set them to
bool FFBConfig::LoadIniSettings(const std::wstring& filename)
{
    std::wifstream file(filename.c_str());
    if (!file.is_open())
    {
        LogMessage(L"[INFO] No ini file found, creating default log.txt");
        WriteFFBIniFile();
        file.open(filename.c_str());
        if (!file.is_open()) { return false; }
    }

    std::wstring currentSection = L"none";

    std::wstring line;
    while (std::getline(file, line))
    {
        ParseLine(currentSection, line);
    }
    if (GetBool(L"base", L"verbose"))
    {
        LogConfig();
    }
    return !GetString(L"base", L"device").empty();
}

int FFBConfig::LoadFFBSettings()
{
    // Load FFB configuration file "ffb.ini"
    if (!LoadIniSettings(L"ffb.ini"))
    {
        LogMessage(L"[ERROR] Failed to load FFB settings from ffb.ini");
        LogMessage(L"[ERROR] A ffb.ini was written to the current working directory. Change changes and start this program again");

        // SHOW ERROR ON CONSOLE immediately
        std::wcout << L"[ERROR] Failed to load FFB settings from ffb.ini" << L'\n';
        std::wcout << L"[ERROR] A ffb.ini was written to the current working directory. Change changes and start this program again" << L'\n';
        std::wcout << L"Press any key to exit..." << L'\n';
        std::cin.get();
        return 1;
    }

    LogMessage(L"[INFO] Successfully loaded FFB settings");
    LogMessage(L"[INFO] Target device: " + GetString(L"base", L"device"));

    game = RequestedGame(GetString(L"base", L"game"));
    LogMessage(L"[INFO] Configured game: " + game.ToString());
    return 0;
}

FFBConfig::Setting::Setting(const std::wstring& key, bool defaultValue, const std::wstring& description) :
    mKey(key),
    mDescription(description),
    mValue(defaultValue) {}

FFBConfig::Setting::Setting(const std::wstring& key, const wchar_t* defaultValue, const std::wstring& description) :
    mKey(key),
    mDescription(description),
    mValue(defaultValue) {}

FFBConfig::Setting::Setting(const std::wstring& key, double defaultValue, const std::wstring& description) :
    mKey(key),
    mDescription(description),
    mValue(defaultValue) {}

FFBConfig::Setting::Setting(const std::wstring& key, int defaultValue, const std::wstring& description) :
    mKey(key),
    mDescription(description),
    mValue(defaultValue) {}

std::wstring FFBConfig::Setting::ToString() const
{
    return mKey + L" = " + mValue.ToString();
}

std::wstring FFBConfig::Setting::Value::ToString() const
{
    switch (mType)
    {
        case ST_BOOL:
            return ToCamelCase((b ? L"true" : L"false"));
        case ST_STRING:
            return s;
        case ST_DOUBLE:
        {
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunsafe-buffer-usage" // swprintf() unsafe
#endif
            wchar_t buffer[32] = {};
            std::swprintf(buffer, 32, L"%.2f", d); // NOLINT(*-vararg)
            // format shortening is highly subjective! adjust this if necessary
            // at the time of writing ini values are percentages,
            // but also support scale values with "pecentage steps" resolution.
            return buffer;
#if defined(__clang__)
#    pragma clang diagnostic pop
#endif
        }
        case ST_INT:
            return std::to_wstring(d);
        default: return L"";
    }
}

bool FFBConfig::Setting::Value::FromString(const std::wstring& valueString)
{
    switch (mType)
    {
        case ST_BOOL:
            b = ToLower(valueString) == L"true";
            return true;
        case ST_STRING:
            s = valueString;
            return true;
        case ST_DOUBLE:
            d = std::stod(valueString);
            return true;
        case ST_INT:
            i = std::stoi(valueString);
            return true;
        default: break;
    }
    return false;
}
