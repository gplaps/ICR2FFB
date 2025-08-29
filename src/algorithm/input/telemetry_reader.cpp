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

// Offsets database for different games/versions
// THANK YOU ERIC

// BOB! Bobby Rahal unlocks it all. Find where the text for licensing him is and work from there
// Provides standardized 'point' to reference for memory
// Maybe this can be replaced with something else more reliable and something that stays the same no matter the game version?
#define ICR2SIG      "license with Bob"
#define ICR2SIG_REND "Use Rendition" // or search for "-gRN1f" command line switch
#define NR1SIG       "name of Harry Gant"
#define NR2SIG       "NASCAR V2.03"
#define UNINIT_SIG   "TEXT_THAT_SHOULD_NOT_BE_IN_ANY_BINARY_N0Txt2BFouND"

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

    ICR2SIG //offset base
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

    ICR2SIG
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

     ICR2SIG
};

//N1 Offsets
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

    NR1SIG
};

//N2 Offsets
static const GameOffsets Offsets_NASCAR2 = {
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

    NR2SIG
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

    UNINIT_SIG
};

void GameOffsets::ApplySignature(uintptr_t signatureAddress)
{
    const uintptr_t exeBase = signatureAddress - signature;
    signature         = exeBase;

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

static GameOffsets GetGameOffsets(GameVersion version)
{
    switch (version)
    {
        case ICR2_DOS4G_1_02:
            return Offsets_ICR2_DOS;
        case ICR2_RENDITION:
            return Offsets_ICR2_REND;
        case ICR2_WINDOWS:
            return Offsets_ICR2_WINDY;
        case NASCAR1:
            return Offsets_NASCAR;
        case NASCAR2_V2_03:
            return Offsets_NASCAR2;
        case VERSION_UNINITIALIZED:
        default:
            return Offsets_Unspecified;
    }
}

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
    offsets(),
    out(),
    rawData(),
    carData()
{
    if (config.GetString(L"base", L"game").empty())
    {
        LogMessage(L"[ERROR] \"Game\" is not set.");
        return;
    }

    // Find process window
    const DWORD pid = FindProcessIdByWindow(config.version);
    if (!pid)
    {
        LogMessage(L"[ERROR] Game window not found.");
        return;
    }

    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) { return; }

    // it should be possible to detect any supported game by searching the memory for known signatures and selecting one of the known offsets for it without the need for the user to specify it
    // if more game offsets have been found this project likely has to undergo a (significant) rewrite
    offsets                 = GetGameOffsets(config.version);
    const uintptr_t signatureAddress = ScanSignature(hProcess, offsets);
    if (!signatureAddress)
    {
        CloseHandle(hProcess);
        hProcess = NULL;
        return;
    }

    offsets.ApplySignature(signatureAddress);

    LogMessage(L"[INIT] EXE base: 0x" + std::to_wstring(offsets.signature) +
               L" | cars_data @ 0x" + std::to_wstring(offsets.cars_data) + 
               L" | rf_mag_lat @ 0x" + std::to_wstring(offsets.tire_maglat_fr));

    /*
    if (config.version == ICR2_DOS4G_1_02 || config.version == ICR2_RENDITION) {

        LogMessage(L"=== ICR2 MEMORY ANALYSIS ===");

        // Log the found signature address
        LogMessage(L"[ICR2] Signature found at: 0x" + std::to_wstring(signatureAddress));

        // Log the signature offset being used
        LogMessage(L"[ICR2] Using signature offset: 0x" + std::to_wstring(offsets.signature));

        // Calculate and log the EXE base
        uintptr_t calculatedExeBase = signatureAddress - offsets.signature;
        LogMessage(L"[ICR2] Calculated EXE base: 0x" + std::to_wstring(calculatedExeBase));

        // Known values from your analysis
        uintptr_t knownFileOffset = 0xF21CC;  // From HxD
        uintptr_t knownCheatEngineAddr = 0x1054BD98;  // From Cheat Engine
        uintptr_t knownsignature = 0xA0D78;  // From working code

        LogMessage(L"[ICR2] Known file offset: 0x" + std::to_wstring(knownFileOffset));
        LogMessage(L"[ICR2] Known Cheat Engine addr: 0x" + std::to_wstring(knownCheatEngineAddr));
        LogMessage(L"[ICR2] Known signature offset: 0x" + std::to_wstring(knownsignature));

        // Calculate various relationships
        uintptr_t memoryToFileOffset = signatureAddress - knownFileOffset;
        LogMessage(L"[ICR2] Memory to file offset diff: 0x" + std::to_wstring(memoryToFileOffset));

        uintptr_t baseFromFile = knownCheatEngineAddr - knownFileOffset;
        LogMessage(L"[ICR2] Base calculated from file: 0x" + std::to_wstring(baseFromFile));

        uintptr_t baseFromSignature = knownCheatEngineAddr - knownsignature;
        LogMessage(L"[ICR2] Base calculated from signature: 0x" + std::to_wstring(baseFromSignature));

        // Test if our current calculation matches the working method
        bool calculationMatches = (calculatedExeBase == baseFromSignature);
        LogMessage(L"[ICR2] Current calculation matches working method: " + std::wstring(calculationMatches ? L"YES" : L"NO"));

        // Show the actual working car data address
        uintptr_t workingCarDataAddr = calculatedExeBase + offsets.cars_data;
        LogMessage(L"[ICR2] Working car data address: 0x" + std::to_wstring(workingCarDataAddr));

        // Calculate what the NASCAR signature offset should be using the same relationship
        uintptr_t nascarFileOffset = 0xF1C69;  // NASCAR's file offset from HxD
        uintptr_t nascarMemoryAddr = 0x10916635;  // NASCAR's memory address from your search

        // Method 1: Use the same memory-to-file relationship
        uintptr_t nascarSigOffset1 = nascarMemoryAddr - nascarFileOffset;
        LogMessage(L"[NASCAR CALC 1] Using memory-file diff: 0x" + std::to_wstring(nascarSigOffset1));

        // Method 2: Use the same base calculation method
        uintptr_t icr2BaseOffset = baseFromSignature - knownCheatEngineAddr;
        uintptr_t nascarSigOffset2 = nascarMemoryAddr + icr2BaseOffset;
        LogMessage(L"[NASCAR CALC 2] Using base offset method: 0x" + std::to_wstring(nascarSigOffset2));

        // Method 3: Direct signature offset calculation
        uintptr_t nascarSigOffset3 = nascarMemoryAddr - (calculatedExeBase - signatureAddress + nascarMemoryAddr);
        LogMessage(L"[NASCAR CALC 3] Direct calculation: Need to determine correct base");

        LogMessage(L"=== END ICR2 ANALYSIS ===");
        */
    //}

    // LogMessage(L"[DEBUG] Selected signature: 0x" + std::to_wstring(offsets.signature));
    // LogMessage(L"[DEBUG] Selected cars_data: 0x" + std::to_wstring(offsets.cars_data));
    // LogMessage(L"[DEBUG] Raw calculation: 0x" + std::to_wstring(signatureAddress) + L" - 0x" + std::to_wstring(offsets.signature) + L" + 0x" + std::to_wstring(offsets.cars_data));

    //temp debug
    // LogMessage(L"[DEBUG] Signature found at: 0x" + std::to_wstring(signatureAddress));
    // LogMessage(L"[DEBUG] Calculated EXE base: 0x" + std::to_wstring(exeBase));
    // LogMessage(L"[DEBUG] Calculated car data addr: 0x" + std::to_wstring(carsDataAddr));
    // LogMessage(L"[DEBUG] Expected car data addr: 0x1058474C");

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

bool TelemetryReader::ReadRaw(void* dest, uintptr_t src, SIZE_T size)
{
    SIZE_T bytesRead = 0;
    return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(src), dest, size, &bytesRead) && bytesRead == size;
}

// === Main ===

void TelemetryReader::ConvertCarData()
{
    // Set variables to be used everywhere else
    // Little bit of math to make the data sensible. Save big calculations for specific "Calculation" sets
    out.pos.dlong        = static_cast<double>(carData.data[4]);
    out.pos.dlat         = static_cast<double>(carData.data[5]);
    out.pos.rotation_deg = static_cast<double>(carData.data[7]) / 2147483648.0 /*static_cast<double>(INT_MAX - 1)*/ * 180.0;

    out.speed_mph        = static_cast<double>(carData.data[8]) / 75.0;

    out.steering_deg     = static_cast<double>(carData.data[10]) / 11600000.0;
    out.steering_raw     = static_cast<double>(carData.data[10]);
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
    if (!ReadRaw(&carData, offsets.cars_data, sizeof(CarData)))
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
        ReadValue(rawData.loadLF, offsets.tire_data_fl) &&
        ReadValue(rawData.loadRF, offsets.tire_data_fr) &&
        ReadValue(rawData.loadLR, offsets.tire_data_lr) &&
        ReadValue(rawData.loadRR, offsets.tire_data_rr) &&

        ReadValue(rawData.magLatLF, offsets.tire_maglat_fl) &&
        ReadValue(rawData.magLatRF, offsets.tire_maglat_fr) &&
        ReadValue(rawData.magLatLR, offsets.tire_maglat_lr) &&
        ReadValue(rawData.magLatRR, offsets.tire_maglat_rr) &&

        ReadValue(rawData.magLongLF, offsets.tire_maglong_fl) &&
        ReadValue(rawData.magLongRF, offsets.tire_maglong_fr) &&
        ReadValue(rawData.magLongLR, offsets.tire_maglong_lr) &&
        ReadValue(rawData.magLongRR, offsets.tire_maglong_rr);

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
