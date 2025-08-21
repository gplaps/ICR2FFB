#include "telemetry_reader.h"

#include "ffb_config.h"
#include "log.h"
#include "string_utilities.h"

#include <psapi.h>
#if !defined(IS_CPP11_COMPLIANT)
#    include <stdint.h>
#endif
#include <tlhelp32.h>

#include <cwctype>
#include <sstream>
#include <vector>

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

// Offsets for different version of the game

// BOB! Bobby Rahal unlocks it all. Find where the text for licensing him is and work from there
// Provides standardized 'point' to reference for memory
// Maybe this can be replaced with something else more reliable and something that stays the same no matter the game version?
#define ICR2SIG      "license with Bob"
#define ICR2SIG_REND "Use Rendition" // or search for "-gRN1f" command line switch
#define UNINIT_SIG   "TEXT_THAT_SHOULD_NOT_BE_IN_ANY_BINARY_N0Txt2BFouND"

// Rendition EXE
static const GameOffsets ICR2_Offsets_REND = {
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

    ICR2SIG //offset base
};

// DOS4G Exe, should be 1.02
static const GameOffsets ICR2_Offsets_DOS = {
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

    ICR2SIG};

static const GameOffsets Unspecified_Offsets = {
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

    UNINIT_SIG};

void GameOffsets::ApplySignature(uintptr_t sigAddr)
{
    const uintptr_t exeBase = sigAddr - signatureOffset;
    signatureOffset         = exeBase;

    cars_data_offset += exeBase;

    tire_data_offsetfl += exeBase;
    tire_data_offsetfr += exeBase;
    tire_data_offsetrl += exeBase;
    tire_data_offsetrr += exeBase;

    tire_maglat_offsetfl += exeBase;
    tire_maglat_offsetfr += exeBase;
    tire_maglat_offsetrl += exeBase;
    tire_maglat_offsetrr += exeBase;

    tire_maglong_offsetfl += exeBase;
    tire_maglong_offsetfr += exeBase;
    tire_maglong_offsetrl += exeBase;
    tire_maglong_offsetrr += exeBase;
}

static GameOffsets GetGameOffsets(GameVersion version)
{
    switch (version)
    {
        case ICR2_DOS4G_1_02:
            return ICR2_Offsets_DOS;
        case ICR2_RENDITION:
            return ICR2_Offsets_REND;
        case VERSION_UNINITIALIZED:
        default:
            return Unspecified_Offsets;
    }
}

struct FindWindowData
{
    FindWindowData(const std::vector<std::wstring>& keyWords, const std::vector<std::wstring>& excludedKeyWords, DWORD processId) :
        keywords(keyWords),
        excludedkeyWords(excludedKeyWords),
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
    if (titleStr.size())
    {
        LogMessage(L"[DEBUG] Checking window \"" + titleStr + L"\"");
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
static DWORD FindProcessIdByWindow(const std::vector<std::wstring>& keywords, const std::vector<std::wstring>& excludedKeywords)
{
    FindWindowData data(keywords, excludedKeywords, 0);
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

TelemetryReader::TelemetryReader(const FFBConfig& config) :
    hProcess(NULL),
    mInitialized(false),
    offs(),
    out(),
    rawData(),
    carData()
{
    if (config.GetString(L"base", L"game").empty())
    {
        LogMessage(L"[ERROR] \"Game\" is not set.");
        return;
    }

    // Keywords to find game. "dosbox" + whatever is in the ini as "Game:"
    std::vector<std::wstring> keywords;
    keywords.push_back(L"dosbox");
    keywords.push_back(config.GetString(L"base", L"game"));
    std::vector<std::wstring> excludedKeywords;
    excludedKeywords.push_back(L"rready");        // Rendition wrapper window
    excludedKeywords.push_back(L"speedy3d");      // Rendition wrapper window
    excludedKeywords.push_back(L"status window"); // DosBox status window
    const DWORD pid = FindProcessIdByWindow(keywords, excludedKeywords);
    if (!pid)
    {
        LogMessage(L"[ERROR] Game window not found.");
        return;
    }

    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) { return; }

    // it should be possible to detect any supported game by searching the memory for known signatures and selecting one of the known offsets for it without the need for the user to specify it
    // if more game offsets have been found this project likely has to undergo a (significant) rewrite
    offs                    = GetGameOffsets(config.version);
    const uintptr_t sigAddr = ScanSignature(hProcess, offs);
    if (!sigAddr)
    {
        CloseHandle(hProcess);
        hProcess = NULL;
        return;
    }

    offs.ApplySignature(sigAddr);

    LogMessage(L"[INIT] EXE base: 0x" + std::to_wstring(offs.signatureOffset) +
               L" | cars_data @ 0x" + std::to_wstring(offs.cars_data_offset));

    mInitialized = true;
}

TelemetryReader::~TelemetryReader()
{
    if (hProcess)
    {
        CloseHandle(hProcess);
        hProcess = NULL;
    }
}

bool TelemetryReader::Initialized() const
{
    return mInitialized;
}

bool TelemetryReader::Valid() const
{
    return Initialized() && hProcess;
}

const RawTelemetry& TelemetryReader::Data() const
{
    return out;
}

bool TelemetryReader::ReadRaw(void* dest, uintptr_t offset, SIZE_T size)
{
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(offset), dest, size, &bytesRead) && bytesRead == size;
}

// === Main ===

void TelemetryReader::ConvertCarData()
{
    // Set variables to be used everywhere else
    // Little bit of math to make the data sensible. Save big calculations for specific "Calculation" sets
    out.dlong        = static_cast<double>(carData.data[4]);
    out.dlat         = static_cast<double>(carData.data[5]);
    out.rotation_deg = static_cast<double>(carData.data[7]) / static_cast<double>(INT_MAX - 1) * 180.0;
    out.speed_mph    = static_cast<double>(carData.data[8]) / 75.0;
    out.steering_deg = static_cast<double>(carData.data[10]) / 11600000.0;
    out.steering_raw = static_cast<double>(carData.data[10]);
}

void TelemetryReader::ConvertTireData()
{
    // Tire data! Probably not loads, we dont know what it is

    out.tireload_lf    = static_cast<double>(rawData.loadLF);
    out.tireload_rf    = static_cast<double>(rawData.loadRF);
    out.tireload_lr    = static_cast<double>(rawData.loadLR);
    out.tireload_rr    = static_cast<double>(rawData.loadRR);

    out.tiremaglat_lf  = static_cast<double>(rawData.magLatLF);
    out.tiremaglat_rf  = static_cast<double>(rawData.magLatRF);
    out.tiremaglat_lr  = static_cast<double>(rawData.magLatLR);
    out.tiremaglat_rr  = static_cast<double>(rawData.magLatRR);

    out.tiremaglong_lf = static_cast<double>(rawData.magLongLF);
    out.tiremaglong_rf = static_cast<double>(rawData.magLongRF);
    out.tiremaglong_lr = static_cast<double>(rawData.magLongLR);
    out.tiremaglong_rr = static_cast<double>(rawData.magLongRR);
}

bool TelemetryReader::ReadCarData()
{
    if (!ReadRaw(&carData, offs.cars_data_offset, sizeof(CarData)))
    {
        LogMessage(L"[ERROR] Failed to read car0 data. GetLastError(): " + std::to_wstring(GetLastError()));
        CloseHandle(hProcess);
        hProcess = NULL;
        return false;
    }

    ConvertCarData();
    return true;
}

bool TelemetryReader::ReadTireData()
{
    const bool tireOK =
        ReadValue(rawData.loadLF, offs.tire_data_offsetfl) &&
        ReadValue(rawData.loadRF, offs.tire_data_offsetfr) &&
        ReadValue(rawData.loadLR, offs.tire_data_offsetrl) &&
        ReadValue(rawData.loadRR, offs.tire_data_offsetrr) &&

        ReadValue(rawData.magLatLF, offs.tire_maglat_offsetfl) &&
        ReadValue(rawData.magLatRF, offs.tire_maglat_offsetfr) &&
        ReadValue(rawData.magLatLR, offs.tire_maglat_offsetrl) &&
        ReadValue(rawData.magLatRR, offs.tire_maglat_offsetrr);

    ReadValue(rawData.magLongLF, offs.tire_maglong_offsetfl) &&
        ReadValue(rawData.magLongRF, offs.tire_maglong_offsetfr) &&
        ReadValue(rawData.magLongLR, offs.tire_maglong_offsetrl) &&
        ReadValue(rawData.magLongRR, offs.tire_maglong_offsetrr);

    if (!tireOK)
    {
        LogMessage(L"[ERROR] Failed to read one or more tire loads.");
        return false;
    }
    ConvertTireData();
    return true;
}

bool TelemetryReader::Update()
{
    out.valid =
        ReadCarData() &&
        ReadTireData();
    return out.valid;
}
