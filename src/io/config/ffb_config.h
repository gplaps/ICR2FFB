#pragma once

#include "game_version.h"

#include <map>
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
    bool         GetBool(const std::wstring& section, const std::wstring& key) const;
    std::wstring GetString(const std::wstring& section, const std::wstring& key) const;
    double       GetDouble(const std::wstring& section, const std::wstring& key) const;
    int          GetInt(const std::wstring& section, const std::wstring& key) const;

private:
    void RegisterSettings();
    bool LoadFFBSettings(const std::wstring& filename);
    void WriteIniFile();
    void LogConfig();
    bool ParseLine(std::wstring& currentSection, const std::wstring& line);

    // this can be achieved elegantly with std::any / boost::variant in C++17 to let it do the runtime conversion, here its implemented not as flexible
    // template<typename T>
    // T         GetSetting(const std::wstring& key) const {}
    // template<typename T>
    // T         SetSetting(const std::wstring& key) {}

    enum SettingType
#if defined(IS_CPP11_COMPLIANT)
        : unsigned char
#endif
    {
        ST_BOOL,
        ST_STRING,
        ST_DOUBLE,
        ST_INT
    };

    struct Setting
    {
        Setting(const std::wstring& key, const wchar_t* defaultValue, const std::wstring& description);
        Setting(const std::wstring& key, bool defaultValue, const std::wstring& description);
        Setting(const std::wstring& key, double defaultValue, const std::wstring& description);
        Setting(const std::wstring& key, int defaultValue, const std::wstring& description);

        std::wstring mKey;
        std::wstring mDescription;
        struct SettingValue
        {
            SettingValue() :
                b(),
                s(),
                d(),
                i(),
                mType(ST_BOOL) {} // undecided if defaulting to bool is ok
            explicit SettingValue(bool fromBool) :
                b(fromBool),
                s(),
                d(),
                i(),
                mType(ST_BOOL) {}
            explicit SettingValue(const wchar_t* FromString) :
                b(),
                s(FromString),
                d(),
                i(),
                mType(ST_STRING) {}
            explicit SettingValue(double fromDouble) :
                b(),
                s(),
                d(fromDouble),
                i(),
                mType(ST_DOUBLE) {}
            explicit SettingValue(int fromInt) :
                b(),
                s(),
                d(),
                i(fromInt),
                mType(ST_INT) {}
            bool         b;
            std::wstring s;
            double       d;
            int          i;

            SettingType mType;

            std::wstring ToString() const;
            bool         FromString(const std::wstring& valueString);
        };
        SettingValue mValue;

        std::wstring ToString() const;
    };
    std::map<std::wstring, std::vector<Setting> > settings;
    std::vector<std::pair<std::wstring, std::wstring>> sectionDescription;
    const Setting& GetSetting(const std::wstring& section, const std::wstring& key) const;
};
