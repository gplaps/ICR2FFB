#include "game_detect.h"

#include "game_version.h"

#include <algorithm>
#include <cwchar>
#include <string>
#include <vector>

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
            result.first.push_back(ICR2SIG_ALL_VERSIONS);

            result.second.push_back(ICR2SIG_REND);
            break;
        }
        case ICR2_RENDITION:
        {
            result.first.push_back(ICR2SIG_ALL_VERSIONS);
            result.first.push_back(ICR2SIG_REND);
            break;
        }
        case ICR2_WINDOWS:
        {
            // not implemented
            result.first.push_back(ICR2SIG_ALL_VERSIONS);

            result.second.push_back(ICR2SIG_REND);
            LogMessage(L"[WARNING] Game detection of ICR2 Windows version not implemented");
            break;
        }
        case NASCAR1:
        {
            result.first.push_back(NR1SIG);
            break;
        }
        case NASCAR2:
        {
            result.first.push_back(NR2SIG);
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
            result.push_back(ICR2SIG_ALL_VERSIONS);
            break;
        }
        case ICR2_RENDITION:
        {
            result.push_back(ICR2SIG_ALL_VERSIONS);
            result.push_back(ICR2SIG_REND);
            break;
        }
        case ICR2_WINDOWS:
        {
            result.push_back(ICR2SIG_ALL_VERSIONS);
            result.push_back(ICR2SIG_WINDY);
            break;
        }
        case NASCAR1:
        {
            result.push_back(NR1SIG);
            break;
        }
        case NASCAR2:
        {
            result.push_back(NR2SIG);
            break;
        }
        case AUTO_DETECT:
        {
            result.push_back(ICR2SIG_ALL_VERSIONS);
            result.push_back(ICR2SIG_REND);
            result.push_back(ICR2SIG_WINDY);
            result.push_back(NR1SIG);
            result.push_back(NR2SIG);
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
    return elem.second == ICR2SIG_ALL_VERSIONS;
}

static bool IsIcr2Rend(const std::pair<uintptr_t, std::string>& elem)
{
    return elem.second == ICR2SIG_REND;
}

static bool IsIcr2Windy(const std::pair<uintptr_t, std::string>& elem)
{
    return elem.second == ICR2SIG_WINDY;
}

static bool IsNR1(const std::pair<uintptr_t, std::string>& elem)
{
    return elem.second == NR1SIG;
}

static bool IsNR2(const std::pair<uintptr_t, std::string>& elem)
{
    return elem.second == NR2SIG;
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
