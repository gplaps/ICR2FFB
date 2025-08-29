#pragma once

#include "game_version.h"

#include <map>
#include <string>
#include <vector>

/* simple project specific ini parser */
struct FFBConfig
{
    FFBConfig();
    bool          Valid() const;
    SupportedGame game;

    //settings from the ffb.ini
    bool         GetBool(const std::wstring& section, const std::wstring& key) const;
    std::wstring GetString(const std::wstring& section, const std::wstring& key) const;
    double       GetDouble(const std::wstring& section, const std::wstring& key) const;
    int          GetInt(const std::wstring& section, const std::wstring& key) const;
    // no boost/std::any / boost/std::variant before C++17

private:
    void RegisterSettings();
    // Search the ini file for settings and find what the user has set them to
    int  LoadFFBSettings();
    bool LoadIniSettings(const std::wstring& filename);
    void WriteFFBIniFile() const;
    void LogConfig() const;
    bool ParseLine(std::wstring& currentSection, const std::wstring& line);

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
        struct Value
        {
            Value() :
                b(),
                s(),
                d(),
                i(),
                mType(ST_BOOL) {} // undecided if defaulting to bool is ok
            explicit Value(bool fromBool) :
                b(fromBool),
                s(),
                d(),
                i(),
                mType(ST_BOOL) {}
            explicit Value(const wchar_t* FromString) :
                b(),
                s(FromString),
                d(),
                i(),
                mType(ST_STRING) {}
            explicit Value(double fromDouble) :
                b(),
                s(),
                d(fromDouble),
                i(),
                mType(ST_DOUBLE) {}
            explicit Value(int fromInt) :
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
        Value mValue;

        std::wstring ToString() const;
    };
    std::map<std::wstring, std::vector<Setting> >       settings;
    std::vector<std::pair<std::wstring, std::wstring> > sectionDescription;
    const Setting&                                      GetSetting(const std::wstring& section, const std::wstring& key) const;
};
