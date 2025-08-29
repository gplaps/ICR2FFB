#include "game_detect.h"

static std::pair<std::vector<std::wstring>,std::vector<std::wstring>> GetKeywordsForGame(GameVersion version)
{
    std::pair<std::vector<std::wstring>,std::vector<std::wstring>> result; // first == keywords, second == excludedKeywords

    switch(version)
    {
        case ICR2_DOS4G_1_02:
        {
            result.first.push_back(L"dosbox");

            result.first.push_back(L"indycar");
            result.first.push_back(L"cart");

            result.second.push_back(L"rendtion");      // ICR2 Rendition version
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
            LogMessage(L"[INFO] Game detection of ICR2 Windows version not implemented");
            break;
        }
        case NASCAR1: 
        {
            result.first.push_back(L"dosbox");

            result.first.push_back(L"nascar");

            result.second.push_back(L"status window"); // DosBox status window
            break;
        }
        case NASCAR2_V2_03:
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

struct FindWindowData
{
    FindWindowData(const std::pair<std::vector<std::wstring>,std::vector<std::wstring>>& keyWordsAndExcluded, DWORD processId) :
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

// Gets the process ID of indycar
static DWORD FindProcessIdByWindow(GameVersion version)
{
    FindWindowData data(GetKeywordsForGame(version), 0);
    EnumWindows(EnumerateWindowsCallback, reinterpret_cast<LPARAM>(&data));
    return data.pid;
}

// Really don't understand this, but here is where we scan the memory for the data needed
// scanning dosbox memory may not work as expected as once a process was started and is closed it still resides in dosbox processes memory, so maybe the wrong instance / closed instance of a supported game is found and not the most recent / active. it worked if opening and closing the same exe inside dosbox multiple times but its not robust
static uintptr_t ScanSignature(HANDLE processHandle, const GameOffsets& offsets)
{
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunsafe-buffer-usage" // strlen() and memcmp() unsafe
#endif

    std::vector<GameOffsets> knownGameOffsets = GetKnownGameOffsets();
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    LogMessage(L"[DEBUG] Scanning for game...");
    LogMessage(L"[DEBUG] Process min addr: 0x" + std::to_wstring(reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress)));
    LogMessage(L"[DEBUG] Process max addr: 0x" + std::to_wstring(reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress)));

    uintptr_t       addr    = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    const uintptr_t maxAddr = 0x7FFFFFFF;

    MEMORY_BASIC_INFORMATION mbi;
    const size_t             targetLen          = strlen(offsets.signatureStr);
    const size_t             renditionSigLength = strlen(ICR2SIG_REND);
    bool                     isRendition        = false;

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
                    // edge case: search string is across region boundary - this case is not covered

                    // scan for rendition text - as its before the common search string, it should be found before the next loop may exit
                    for (SIZE_T i = 0; i <= bytesRead - renditionSigLength; ++i)
                    {
                        if (memcmp(buffer.data() + i, ICR2SIG_REND, renditionSigLength) == 0) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        {
                            isRendition = true;
                        }
                    }

                    for (SIZE_T i = 0; i <= bytesRead - targetLen; ++i)
                    {
                        if (memcmp(buffer.data() + i, offsets.signatureStr, targetLen) == 0) // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        {
                            std::wstringstream ss;
                            ss << L"[MATCH] Found Game at 0x" << std::hex << (addr + i);
                            if (isRendition)
                            {
                                ss << L" in the rendition version";
                            }
                            LogMessage(ss.str());
                            return addr + i;
                        }
                    }
                }
            }
            addr += mbi.RegionSize;
        }
        else
        {
            addr += 0x1000;
        }
    }

    LogMessage(L"[ERROR] Signature not found in game.");
    return 0;

#if defined(__clang__)
#    pragma clang diagnostic pop
#endif
}
