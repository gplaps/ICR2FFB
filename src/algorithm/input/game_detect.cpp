#include "game_detect.h"

#include "game_version.h"

#include <algorithm>
#include <cwchar>
#include <string>
#include <vector>

// Game detection and offsets are a work in progress and only very specific versions are detected in this version
// This has to undergo a rewrite if more versions are added

// Offsets database for different games/versions
// THANK YOU ERIC

// Provides standardized 'point' to reference for memory
static const char* DOS_EXTENDER_DOS32A   = "DOS/32A";
static const char* DOS_EXTENDER_DOS4G    = "DOS/4G";

static const char* ICR2_SIG_ALL_VERSIONS = "license with Bob";
static const char* ICR2_V1_00            = "Version 1.0.0"; // exists, offsets unknown
static const char* ICR2_V1_02            = "Version 1.0.2"; // offsets valid for this version
static const char* ICR2_SIG_REND         = "-RN1 Build";
static const char* ICR2_SIG_WINDY        = "<Insert text that only is found in the Windows version of ICR2";

static const char* NR1_SIG               = "name of Harry Gant";
static const char* NR1_V1_21             = "Version 1.21";

static const char* NR2_SIG               = "NASCAR V2.03"; // too specific - this and some of the game detection mechanism has to be changed if more binaries and their offsets are known
static const char* NR2_V1_005            = "Version 1.00(5)";
static const char* NR2_RENDITION         = "Rendition communication timeout";

static const char* UNINIT_SIG            = "TEXT_THAT_SHOULD_NOT_BE_IN_ANY_BINARY_N0Txt2BFouND";

// Rendition EXE
static const GameOffsets Offsets_ICR2_REND = {
    0xB1C0C, //signature

    0xE0EA4, //cars data

    0xBB4E8, //lf tire load?
    0xBB4EA, //rf tire load?
    0xBB4E4, //lr tire load?
    0xBB4E6, //rr tire load?

    0xEAB24, //lf tire lat load
    0xEAB26, //rf tire lat load
    0xEAB20, //lr tire lat load
    0xEAB22, //rr tire lat load

    0xEAB04, //lf tire long load
    0xEAB06, //rf tire long load
    0xEAB00, //lr tire long load
    0xEAB02, //rr tire long load

    ICR2_SIG_ALL_VERSIONS //offset base
};

// DOS4G Exe, should be 1.02
static const GameOffsets Offsets_ICR2_DOS = {
    0xA0D78,

    0xD4718,

    0xA85B8,
    0xA85BA,
    0xA85B4,
    0xA85B6,

    0xC5C48,
    0xC5C4A,
    0xC5C44,
    0xC5C46,

    0xC5C18,
    0xC5C1A,
    0xC5C14,
    0xC5C16,

    ICR2_SIG_ALL_VERSIONS //offset base
};

// ICR2 Windy
static const GameOffsets Offsets_ICR2_WINDY = {
    0x004E2161,

    0x004E0000, /* ???? */ // missing cars data

    0x004F3854,
    0x004F3856,
    0x004F3850,
    0x004F3852,

    0x00528204,
    0x00528206,
    0x00528200,
    0x00528202,

    0x005281F8,
    0x005281Fa,
    0x005281F4,
    0x005281F6,

    ICR2_SIG_ALL_VERSIONS //offset base
};

// N1 Offsets
static const GameOffsets Offsets_NASCAR = {
    0xAEA8C,

    0xEFED4,

    0xCEF70,
    0xCEF70,
    0xCEF70,
    0xCEF70,

    0x9F6F8,
    0x9F6FA,
    0x9f780,
    0x9F6F6,

    0xF0970,
    0xF0970,
    0xF0970,
    0xF0970,

    NR1_SIG //offset base
};

// N2 Offsets
static const GameOffsets Offsets_NASCAR2_V2_03 = {
    0xD7125, // "NASCAR V2.03"

    0xAD440,

    0xF39FA,
    0xF39FC,
    0xF39F6,
    0xF39F8,

    0xF3B0E,
    0xF3B10,
    0xF3B0A,
    0xF3B0C,

    0xF3B02,
    0xF3B04,
    0xF3AFE,
    0xF3B00,

    NR2_SIG //offset base
};

// Not found
static const GameOffsets Offsets_Unspecified = {
    0x0,

    0x0,

    0x0,
    0x0,
    0x0,
    0x0,

    0x0,
    0x0,
    0x0,
    0x0,

    0x0,
    0x0,
    0x0,
    0x0,

    UNINIT_SIG //offset base
};

GameOffsets GetGameOffsets(GameVersion version)
{
    switch (version)
    {
        case ICR2_DOS:
            return Offsets_ICR2_DOS;
        case ICR2_RENDITION:
            return Offsets_ICR2_REND;
        case ICR2_WINDOWS:
            return Offsets_ICR2_WINDY;
        case NASCAR1:
            return Offsets_NASCAR;
        case NASCAR2:
            return Offsets_NASCAR2_V2_03;
        case AUTO_DETECT:
        case VERSION_UNINITIALIZED:
        default:
            return Offsets_Unspecified;
    }
}

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

// ----------------------------------
// Find window
// ----------------------------------

struct FindWindowData
{
    FindWindowData(const std::pair<std::vector<std::wstring>, std::vector<std::wstring> >& keyWordsAndExcluded, DWORD processId) :
        keywords(keyWordsAndExcluded.first),
        excludedkeyWords(keyWordsAndExcluded.second),
        pid(processId) {}
    std::vector<std::wstring> keywords;
    std::vector<std::wstring> excludedkeyWords;
    DWORD                     pid;
};

static BOOL
#if defined(__GNUC__) && !defined(__clang__)
    CALLBACK
#endif
    EnumerateWindowsCallback(HWND hwnd, LPARAM lParam)
{
    FindWindowData* wdata = reinterpret_cast<FindWindowData*>(lParam);
    TCHAR           title[256];
    GetWindowText(hwnd, title, sizeof(title) / sizeof(TCHAR));
    const std::wstring titleStr = ToWideString(title);
    const std::wstring wTitle   = ToLower(titleStr);
    if (!titleStr.empty())
    {
        // LogMessage(L"[DEBUG] Checking window \"" + titleStr + L"\"");
        for (size_t i = 0; i < wdata->keywords.size(); ++i)
        {
            const std::wstring& key = ToLower(wdata->keywords[i]);
            if (wTitle.find(key) != std::wstring::npos)
            {
                for (size_t j = 0; j < wdata->excludedkeyWords.size(); ++j)
                {
                    const std::wstring& exKey = ToLower(wdata->excludedkeyWords[j]);
                    // LogMessage(L"[DEBUG] Checking exclude: \"" + exKey + L"\"");
                    if (wTitle.find(exKey) != std::wstring::npos)
                    {
                        LogMessage((L"[DEBUG] Skipping window \"" + titleStr) + (L"\" because it contains \"" + exKey + L'\"'));
                        return TRUE;
                    }
                }
                LogMessage((L"[DEBUG] Window \"" + wTitle) + (L"\" matches \"" + key + L'\"'));
                GetWindowThreadProcessId(hwnd, &wdata->pid);
                return FALSE;
            }
        }
    }

    return TRUE;
}

static std::pair<std::vector<std::wstring>, std::vector<std::wstring> > GetKeywordsForGameWindow(GameVersion version)
{
    std::pair<std::vector<std::wstring>, std::vector<std::wstring> > result; // first == keywords, second == excludedKeywords

    switch (version)
    {
        case ICR2_DOS:
        {
            result.first.push_back(L"dosbox");

            result.first.push_back(L"indycar");
            result.first.push_back(L"cart");

            result.second.push_back(L"rendtion");      // ICR2 Rendition version
            result.second.push_back(L"rready");        // Rendition wrapper window
            result.second.push_back(L"speedy3d");      // Rendition wrapper window
            result.second.push_back(L"status window"); // DosBox status window

            break;
        }
        case ICR2_RENDITION:
        {
            result.first.push_back(L"dosbox");

            result.first.push_back(L"indycar");
            result.first.push_back(L"cart");

            result.second.push_back(L"rready");        // Rendition wrapper window
            result.second.push_back(L"speedy3d");      // Rendition wrapper window
            result.second.push_back(L"status window"); // DosBox status window
            break;
        }
        case ICR2_WINDOWS:
        {
            // not implemented

            result.first.push_back(L"indycar");
            result.first.push_back(L"cart");

            LogMessage(L"[WARNING] Game window detection of ICR2 Windows version untested");
            break;
        }
        case NASCAR1:
        {
            result.first.push_back(L"dosbox");

            result.first.push_back(L"nascar");

            result.second.push_back(L"status window"); // DosBox status window
            break;
        }
        case NASCAR2:
        {
            result.first.push_back(L"dosbox");

            result.first.push_back(L"nascar");

            result.second.push_back(L"status window"); // DosBox status window
            break;
        }
        case AUTO_DETECT:
        {
            result.first.push_back(L"dosbox");

            result.second.push_back(L"status window"); // DosBox status window
            break;
        }
        case VERSION_UNINITIALIZED:
        default:
            break;
    }
    return result;
}

// Gets the process ID of supported games
DWORD FindProcessIdByWindow(GameVersion version)
{
    FindWindowData data(GetKeywordsForGameWindow(version), 0);
    EnumWindows(EnumerateWindowsCallback, reinterpret_cast<LPARAM>(&data));
    return data.pid;
}

// ----------------------------------
// Detect game and its offsets
// ----------------------------------

static std::pair<std::vector<std::string>, std::vector<std::string> > GetKeywordsForGameDetection(GameVersion version)
{
    std::pair<std::vector<std::string>, std::vector<std::string> > result; // first == keywords, second == excludedKeywords

    switch (version)
    {
        case ICR2_DOS:
        {
            result.first.push_back(ICR2_SIG_ALL_VERSIONS);

            result.second.push_back(ICR2_SIG_REND);
            break;
        }
        case ICR2_RENDITION:
        {
            result.first.push_back(ICR2_SIG_ALL_VERSIONS);
            result.first.push_back(ICR2_SIG_REND);
            break;
        }
        case ICR2_WINDOWS:
        {
            // not implemented
            result.first.push_back(ICR2_SIG_ALL_VERSIONS);

            result.second.push_back(ICR2_SIG_REND);
            LogMessage(L"[WARNING] Game detection of ICR2 Windows version not implemented");
            break;
        }
        case NASCAR1:
        {
            result.first.push_back(NR1_SIG);
            break;
        }
        case NASCAR2:
        {
            result.first.push_back(NR2_SIG);
            break;
        }
        case AUTO_DETECT:
        case VERSION_UNINITIALIZED:
        default:
            break;
    }
    return result;
}

static std::vector<std::string> GetKnownSignatures(GameVersion version)
{
    std::vector<std::string> result;
    switch (version)
    {
        case ICR2_DOS:
        {
            result.push_back(ICR2_SIG_ALL_VERSIONS);
            break;
        }
        case ICR2_RENDITION:
        {
            result.push_back(ICR2_SIG_ALL_VERSIONS);
            result.push_back(ICR2_SIG_REND);
            break;
        }
        case ICR2_WINDOWS:
        {
            result.push_back(ICR2_SIG_ALL_VERSIONS);
            result.push_back(ICR2_SIG_WINDY);
            break;
        }
        case NASCAR1:
        {
            result.push_back(NR1_SIG);
            break;
        }
        case NASCAR2:
        {
            result.push_back(NR2_SIG);
            break;
        }
        case AUTO_DETECT:
        {
            result.push_back(ICR2_SIG_ALL_VERSIONS);
            result.push_back(ICR2_SIG_REND);
            result.push_back(ICR2_SIG_WINDY);
            result.push_back(NR1_SIG);
            result.push_back(NR2_SIG);
            break;
        }
        case VERSION_UNINITIALIZED:
        default:
            break;
    }
    return result;
}

// search predicates
static bool IsIcr2(const std::pair<uintptr_t, std::string>& elem)
{
    return elem.second == ICR2_SIG_ALL_VERSIONS;
}

static bool IsIcr2Rend(const std::pair<uintptr_t, std::string>& elem)
{
    return elem.second == ICR2_SIG_REND;
}

static bool IsIcr2Windy(const std::pair<uintptr_t, std::string>& elem)
{
    return elem.second == ICR2_SIG_WINDY;
}

static bool IsNR1(const std::pair<uintptr_t, std::string>& elem)
{
    return elem.second == NR1_SIG;
}

static bool IsNR2(const std::pair<uintptr_t, std::string>& elem)
{
    return elem.second == NR2_SIG;
}

static std::pair<uintptr_t, GameVersion> DetectGame(const std::vector<std::pair<uintptr_t, std::string> >& scanResult, GameVersion version)
{
    // select from detected keywords
    // GetKeywordsForGameDetection(version);
    std::vector<std::pair<uintptr_t, std::string> >::const_iterator it = scanResult.end();

    std::pair<uintptr_t, GameVersion> detectedVersion(0, VERSION_UNINITIALIZED);
    if (!scanResult.empty())
    {
        it = std::find_if(scanResult.begin(), scanResult.end(), IsIcr2);
        if (it != scanResult.end())
        {
            it = std::find_if(scanResult.begin(), scanResult.end(), IsIcr2Rend);
            if (it != scanResult.end() && (version == ICR2_RENDITION || version == AUTO_DETECT))
            {
                return std::pair<uintptr_t, GameVersion>(it->first, ICR2_RENDITION);
            }
            it = std::find_if(scanResult.begin(), scanResult.end(), IsIcr2Windy);
            if (it != scanResult.end() && (version == ICR2_WINDOWS || version == AUTO_DETECT))
            {
                return std::pair<uintptr_t, GameVersion>(it->first, ICR2_WINDOWS);
            }
            if (version == ICR2_DOS || version == AUTO_DETECT)
            {
                return std::pair<uintptr_t, GameVersion>(it->first, ICR2_DOS);
            }
        }
        it = std::find_if(scanResult.begin(), scanResult.end(), IsNR1);
        if (it != scanResult.end() && (version == NASCAR1 || version == AUTO_DETECT))
        {
            return std::pair<uintptr_t, GameVersion>(it->first, NASCAR1);
        }
        it = std::find_if(scanResult.begin(), scanResult.end(), IsNR2);
        if (it != scanResult.end() && (version == NASCAR2 || version == AUTO_DETECT))
        {
            return std::pair<uintptr_t, GameVersion>(it->first, NASCAR2);
        }
    }
    LogMessage(L"[ERROR] Signature not found in game.");
    return std::pair<uintptr_t, GameVersion>(0x0, VERSION_UNINITIALIZED);
}

// Really don't understand this, but here is where we scan the memory for the data needed
// scanning dosbox memory may not work as expected as once a process was started and is closed it still resides in dosbox processes memory, so maybe the wrong instance / closed instance of a supported game is found and not the most recent / active. it worked if opening and closing the same exe inside dosbox multiple times but its not robust

// ScanSignature scans for known signatures for either a specific version or and tries to even auto detect between different versions if requested (version == AUTO_DETECT)
std::pair<uintptr_t, GameVersion> ScanSignature(HANDLE processHandle, GameVersion version)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    LogMessage(L"[DEBUG] Scanning for game...");
    LogMessage(L"[DEBUG] Process min addr: 0x" + std::to_wstring(reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress)));
    LogMessage(L"[DEBUG] Process max addr: 0x" + std::to_wstring(reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress)));

    uintptr_t       addr    = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    const uintptr_t maxAddr = 0x7FFFFFFF;

    MEMORY_BASIC_INFORMATION mbi;

    const std::vector<std::string>                  signaturesToScan = GetKnownSignatures(version);
    std::vector<BYTE>                               bufferPreviousPage(0);
    std::vector<std::pair<uintptr_t, std::string> > result;

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunsafe-buffer-usage" // memcmp() unsafe
#endif

    while (addr < maxAddr)
    {
        if (VirtualQueryEx(processHandle, reinterpret_cast<LPCVOID>(addr), &mbi, sizeof(mbi)) == sizeof(mbi))
        {
            if ((mbi.State == MEM_COMMIT) && !(mbi.Protect & PAGE_NOACCESS))
            {
                std::vector<BYTE> buffer(mbi.RegionSize);
                SIZE_T            bytesRead = 0;

                if (ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(addr), buffer.data(), mbi.RegionSize, &bytesRead))
                {
                    for (size_t gi = 0; gi < signaturesToScan.size(); ++gi)
                    {
                        const std::string& signature    = signaturesToScan[gi];
                        const size_t       signatureLen = signature.size();

                        // overlap region
                        std::vector<BYTE> overlapRegion;
                        if (!bufferPreviousPage.empty())
                        {
                            overlapRegion = std::vector<BYTE>(bufferPreviousPage.end() - static_cast<ptrdiff_t>(signatureLen), bufferPreviousPage.end());
                            overlapRegion.insert(overlapRegion.end(), buffer.begin(), buffer.begin() + static_cast<ptrdiff_t>(signatureLen));

                            for (SIZE_T i = 0; i < overlapRegion.size(); ++i)
                            {
                                if (memcmp(overlapRegion.data() + i, signature.c_str(), signatureLen) == 0) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                                {
                                    result.push_back(std::pair<uintptr_t, std::string>(addr + i - signatureLen, signature));
                                    break;
                                }
                            }
                        }

                        for (SIZE_T i = 0; i <= bytesRead - signatureLen; ++i)
                        {
                            if (memcmp(buffer.data() + i, signature.c_str(), signatureLen) == 0) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                            {
                                result.push_back(std::pair<uintptr_t, std::string>(addr + i, signature));
                                break;
                            }
                        }
                    }

                    bufferPreviousPage = buffer;
                }
            }
            else
            {
                bufferPreviousPage.clear();
            }
            addr += mbi.RegionSize;
        }
        else
        {
            addr += 0x1000;
            bufferPreviousPage.clear();
        }
    }

#if defined(__clang__)
#    pragma clang diagnostic pop
#endif

    return DetectGame(result, version);
}
