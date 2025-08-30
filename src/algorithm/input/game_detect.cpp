#include "game_detect.h"

#include "game_version.h"
#include "log.h"
#include "string_utilities.h"

#include <cwchar>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// Game detection and offsets are a work in progress and only very specific versions are detected in this version

std::vector<Game>                  SupportedGames::gameList;
std::map<std::string, BaseGame>    SupportedGames::baseGameStrings;
std::map<std::string, Renderer>    SupportedGames::rendererStrings;
std::map<std::string, BinaryType>  SupportedGames::binaryStrings;
std::map<std::string, VersionInfo> SupportedGames::versionStrings;

// Provides standardized 'point' to reference for memory

// Offsets database for different games/versions
// THANK YOU ERIC

// Rendition EXE
static const GameOffsets Offsets_ICR2_REND_DOS32A_102 = {
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
    0xEAB02  //rr tire long load
};

// DOS4G Exe, should be 1.02
static const GameOffsets Offsets_ICR2_DOS4G_102 = {
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
    0xC5C16
};

// ICR2 Windy
static const GameOffsets Offsets_ICR2_WINDY = {
    0x004E2161,

    0x004E0000,
    /* ???? */ // missing cars data

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
    0x005281F6
};

// N1 Offsets - version unknown
static const GameOffsets Offsets_NASCAR1 = {
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
    0xF0970
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
    0xF3B00
};

void InitGameDetection()
{
    SupportedGames::baseGameStrings["license with Bob"]                = INDYCAR_RACING_2;
    SupportedGames::baseGameStrings["name of Harry Gant"]              = NASCAR_RACING_1;
    SupportedGames::baseGameStrings["NASCAR V2.0"]                     = NASCAR_RACING_2;

    SupportedGames::rendererStrings["-RN1 Build"]                      = RENDITION; // ICR2
    SupportedGames::rendererStrings["Rendition communication timeout"] = RENDITION; // NR2

    SupportedGames::binaryStrings["DOS/32A"]                           = DOS32A;
    SupportedGames::binaryStrings["DOS/4G"]                            = DOS4GW;

    SupportedGames::versionStrings["Version 1.0.0"]                    = V1_0_0; // ICR2
    SupportedGames::versionStrings["Version 1.0.2"]                    = V1_0_2; // ICR2
    SupportedGames::versionStrings["Version 1.21"]                     = V1_2_1; // NR1
    SupportedGames::versionStrings["Version 2.0.2"]                    = V2_0_2; // NR2
    SupportedGames::versionStrings["Version 2.0.3"]                    = V2_0_3; // NR2

    SupportedGames::gameList.push_back(Game(INDYCAR_RACING_2, SOFTWARE, V1_0_2, DOS4GW, Offsets_ICR2_DOS4G_102));
    SupportedGames::gameList.push_back(Game(INDYCAR_RACING_2, RENDITION, V1_0_2, DOS32A, Offsets_ICR2_REND_DOS32A_102));
    SupportedGames::gameList.push_back(Game(INDYCAR_RACING_2, SOFTWARE, V1_0_2, WIN32_APPLICATION, Offsets_ICR2_WINDY));
    SupportedGames::gameList.push_back(Game(NASCAR_RACING_1, SOFTWARE, V1_2_1, DOS4GW, Offsets_NASCAR1));
    SupportedGames::gameList.push_back(Game(NASCAR_RACING_2, SOFTWARE, V2_0_3, DOS4GW, Offsets_NASCAR2_V2_03));
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

static std::pair<std::vector<std::wstring>, std::vector<std::wstring> > GetKeywordsForGameWindow()
{
    std::pair<std::vector<std::wstring>, std::vector<std::wstring> > result; // first == keywords, second == excludedKeywords

    result.first.push_back(L"dosbox");

    result.first.push_back(L"indycar");
    result.first.push_back(L"cart");
    result.first.push_back(L"nascar");

    result.second.push_back(L"rready");        // Rendition wrapper window
    result.second.push_back(L"speedy3d");      // Rendition wrapper window
    result.second.push_back(L"status window"); // DosBox status window

    return result;
}

// Gets the process ID of supported games
DWORD FindProcessIdByWindow()
{
    FindWindowData data(GetKeywordsForGameWindow(), 0);
    EnumWindows(EnumerateWindowsCallback, reinterpret_cast<LPARAM>(&data));
    return data.pid;
}

// ----------------------------------
// Detect game and its offsets
// ----------------------------------

static std::vector<std::string> SignaturesToScan()
{
    std::vector<std::string> result;

    // put all known signature strings into a unordered list for scanning
    for (std::map<std::string, BaseGame>::const_iterator it = SupportedGames::baseGameStrings.begin(); it != SupportedGames::baseGameStrings.end(); ++it)
    {
        result.push_back(it->first);
    }
    for (std::map<std::string, Renderer>::const_iterator it = SupportedGames::rendererStrings.begin(); it != SupportedGames::rendererStrings.end(); ++it)
    {
        result.push_back(it->first);
    }
    for (std::map<std::string, BinaryType>::const_iterator it = SupportedGames::binaryStrings.begin(); it != SupportedGames::binaryStrings.end(); ++it)
    {
        result.push_back(it->first);
    }
    for (std::map<std::string, VersionInfo>::const_iterator it = SupportedGames::versionStrings.begin(); it != SupportedGames::versionStrings.end(); ++it)
    {
        result.push_back(it->first);
    }
    return result;
}

// find string and return the corresponding enum and the offset, all other Detect* methods just return the enum
static std::pair<uintptr_t, BaseGame> DetectGame(const std::vector<std::pair<uintptr_t, std::string> >& scanResult)
{
    for (size_t i = 0; i < scanResult.size(); ++i)
    {
        for (std::map<std::string, BaseGame>::const_iterator it = SupportedGames::baseGameStrings.begin(); it != SupportedGames::baseGameStrings.end(); ++it)
        {
            if (it->first == scanResult[i].second)
            {
                return std::pair<uintptr_t, BaseGame>(scanResult[i].first, it->second);
            }
        }
    }
    return std::pair<uintptr_t, BaseGame>(0U, UNDETECTED_GAME);
}

static Renderer DetectRenderer(const std::vector<std::pair<uintptr_t, std::string> >& scanResult)
{
    for (size_t i = 0; i < scanResult.size(); ++i)
    {
        for (std::map<std::string, Renderer>::const_iterator it = SupportedGames::rendererStrings.begin(); it != SupportedGames::rendererStrings.end(); ++it)
        {
            if (it->first == scanResult[i].second)
            {
                return it->second;
            }
        }
    }
    return SOFTWARE;
}

static BinaryType DetectBinaryType(const std::vector<std::pair<uintptr_t, std::string> >& scanResult)
{
    for (size_t i = 0; i < scanResult.size(); ++i)
    {
        for (std::map<std::string, BinaryType>::const_iterator it = SupportedGames::binaryStrings.begin(); it != SupportedGames::binaryStrings.end(); ++it)
        {
            if (it->first == scanResult[i].second)
            {
                return it->second;
            }
        }
    }
    return UNDETECTED_BINARY_TYPE;
}

static VersionInfo DetectVersion(const std::vector<std::pair<uintptr_t, std::string> >& scanResult)
{
    for (size_t i = 0; i < scanResult.size(); ++i)
    {
        for (std::map<std::string, VersionInfo>::const_iterator it = SupportedGames::versionStrings.begin(); it != SupportedGames::versionStrings.end(); ++it)
        {
            if (it->first == scanResult[i].second)
            {
                return it->second;
            }
        }
    }
    return UNDETECTED_VERSION;
}

static Game ToSupportedGame(const std::vector<std::pair<uintptr_t, std::string> >& scanResult)
{
    // translate scan results into a SupportedGame and lookup if this matches any supported version
    if (!scanResult.empty())
    {
        /* Debug */
        for (size_t i = 0; i < scanResult.size(); ++i)
        {
            LogMessage(L"[DEBUG] Scan result contains: " + AnsiToWide(scanResult[i].second.c_str()));
        }

        const std::pair<uintptr_t, BaseGame> game     = DetectGame(scanResult);
        const Renderer                       renderer = DetectRenderer(scanResult);
        const BinaryType                     options  = DetectBinaryType(scanResult);
        const VersionInfo                    version  = DetectVersion(scanResult);

        Game detectedGame                             = SupportedGames::FindGame(game.second, renderer, version, options);
        if (detectedGame.Valid())
        {
            detectedGame.ApplySignature(game.first);
            LogMessage(L"[INFO] Detected game: " + detectedGame.ToString());
            return detectedGame;
        }
        else
        {
            LogMessage(L"[ERROR] Game detected failed. Result: " + detectedGame.ToString());
            return Game();
        }
    }
    LogMessage(L"[ERROR] No signatures found in game.");
    return Game();
}

// Really don't understand this, but here is where we scan the memory for the data needed
// scanning dosbox memory may not work as expected as once a process was started and is closed it still resides in dosbox processes memory, so maybe the wrong instance / closed instance of a supported game is found and not the most recent / active. it worked if opening and closing the same exe inside dosbox multiple times but its not robust
// the "auto detect" scanning mechanism is rather slow as it has to check for quite a few strings! but therefore its convenient
// consider rewriting this again to check for basegame and other options sequentially, ruling out non fitting combinations
Game ScanSignature(HANDLE processHandle)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

	std::wcout << L"Game window found, now scanning process memory. This might take a moment...\n";

    LogMessage(L"[DEBUG] Scanning for game...");
    LogMessage(L"[DEBUG] Process min addr: 0x" + std::to_wstring(reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress)));
    LogMessage(L"[DEBUG] Process max addr: 0x" + std::to_wstring(reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress)));

    uintptr_t       addr    = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    const uintptr_t maxAddr = 0x7FFFFFFF;

    MEMORY_BASIC_INFORMATION mbi;

    std::vector<std::string>                        signaturesToScan = SignaturesToScan();
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
					for (size_t si = 0; si < signaturesToScan.size();)
                    {
                        const std::string& signature      = signaturesToScan[si];
                        const size_t       signatureLen   = signature.size();
                        bool               signatureFound = false;

                        // overlap region
                        std::vector<BYTE> overlapRegion;
                        if (!bufferPreviousPage.empty())
                        {
                            overlapRegion = std::vector<BYTE>(bufferPreviousPage.end() - static_cast<ptrdiff_t>(signatureLen) + 1, bufferPreviousPage.end());
                            overlapRegion.insert(overlapRegion.end(), buffer.begin(), buffer.begin() + static_cast<ptrdiff_t>(signatureLen) - 1);

                            for (SIZE_T i = 0; i <= overlapRegion.size() - signatureLen; ++i)
                            {
                                if (memcmp(overlapRegion.data() + i, signature.c_str(), signatureLen) == 0) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                                {
                                    result.push_back(std::pair<uintptr_t, std::string>(addr + i - signatureLen + 1, signature));
									signatureFound = true;
                                    break;
                                }
                            }
                        }

						if (!signatureFound)
						{
                        for (SIZE_T i = 0; i <= bytesRead - signatureLen; ++i)
                        {
                            if (memcmp(buffer.data() + i, signature.c_str(), signatureLen) == 0) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                            {
                                result.push_back(std::pair<uintptr_t, std::string>(addr + i, signature));
									signatureFound = true;
                                break;
                            }
							}
						}
						if (signatureFound)
						{
							// one less signature to scan to accelerate the rather slow scanning process
                            signaturesToScan.erase(signaturesToScan.begin() + static_cast<ptrdiff_t>(si));
						}
						else
						{
							++si;
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

    return ToSupportedGame(result);
}
