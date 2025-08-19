#pragma once

#include "game_version.h"

#include <string>
#include <vector>

/* project specific ini parser - ini files are not structured in sections */
struct FFBConfig
{
    FFBConfig();
    bool        Valid() const;
    GameVersion version;

    // private:
    // Search the ini file for settings and find what the user has set them to
    int LoadSettingsFromConfig();

    //settings from the ffb.ini
    bool         GetBool(const std::wstring& key) const;
    std::wstring GetString(const std::wstring& key) const;
    double       GetDouble(const std::wstring& key) const;

private:
    void RegisterSettings();
    bool LoadFFBSettings(const std::wstring& filename);
    void WriteIniFile();
    bool ParseLine(const std::wstring& line);

    // this can be achieved elegantly with std::any / boost::variant in C++17 to let it do the runtime conversion, here its implemented not as flexible
    // template<typename T>
    // T         GetSetting(const std::wstring& key) const {}
    // template<typename T>
    // T         SetSetting(const std::wstring& key) {}

    GameVersion  ReadGameVersion(const std::wstring& versionText);
    std::wstring PrintGameVersion() const;

    enum SettingType
#if defined(IS_CPP11_COMPLIANT)
        : unsigned char
#endif
    {
        ST_BOOL,
        ST_STRING,
        ST_DOUBLE
    };

    struct Setting // section unused
    {
        Setting(const std::wstring& section, const std::wstring& key, const wchar_t* defaultValue, const std::wstring& description);
        Setting(const std::wstring& section, const std::wstring& key, bool defaultValue, const std::wstring& description);
        Setting(const std::wstring& section, const std::wstring& key, double defaultValue, const std::wstring& description);
        std::wstring mSection;
        std::wstring mKey;
        std::wstring mDescription;
        struct SettingValue
        {
            SettingValue() :
                b(),
                s(),
                d(),
                mType(ST_BOOL) {} // undecided if defaulting to bool is ok
            explicit SettingValue(bool fromBool) :
                b(fromBool),
                s(),
                d(),
                mType(ST_BOOL) {}
            explicit SettingValue(const wchar_t* FromString) :
                b(),
                s(FromString),
                d(),
                mType(ST_STRING) {}
            explicit SettingValue(double fromDouble) :
                b(),
                s(),
                d(fromDouble),
                mType(ST_DOUBLE) {}
            bool         b;
            std::wstring s;
            double       d;

            SettingType mType;

            std::wstring ToString() const;
            bool         FromString(const std::wstring& valueString);
        };
        SettingValue mValue;

        std::wstring ToString() const;
    };
    std::vector<Setting> settings;
    const Setting&       GetSetting(const std::wstring& key) const;
};
