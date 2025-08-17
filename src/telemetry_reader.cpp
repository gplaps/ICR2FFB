#include "telemetry_reader.h"

#include "project_dependencies.h"

#include "ffb_config.h"
#include "helpers.h"
#include "log.h"

#include <psapi.h>
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

// Rendition EXE
static const GameOffsets Offsets_REND = {
    0xB1C0C, 0xE0EA4, 0xBB4E8, 0xBB4EA, 0xBB4E4, 0xBB4E6, 0xEAB24, 0xEAB26, 0xEAB20, 0xEAB22, 0xEAB00
    //0xB1C0C, 0xE0EA4, 0xBB4E8, 0xBB4EA, 0xBB4E4, 0xBB4E6, 0xEAB16, 0xEAB14, 0xEAB12, 0xEAB10 // original maglat
};

// DOS4G Exe, should be 1.02
static const GameOffsets Offsets_DOS = {
    0xA0D78, 0xD4718, 0xA85B8, 0xA85BA, 0xA85B4, 0xA85B6, 0xC5C48, 0xC5C4A, 0xC5C44, 0xC5C46, 0xC5C14
    //0xA0D78, 0xD4718, 0xA85B8, 0xA85BA, 0xA85B4, 0xA85B6, 0xC5C2A, 0xC5C28, 0xC5C26, 0xC5C24 // original maglat
};

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
    car_longitude_offset += exeBase;
}

static GameOffsets GetGameOffsets(GameVersion version)
{
    switch (version)
    {
        case GameVersion::ICR2_DOS4G_1_02:
            return Offsets_DOS;
        case GameVersion::ICR2_RENDITION:
        case GameVersion::UNINITIALIZED:
        default:
            return Offsets_REND;
    }
}

// BOB! Bobby Rahal unlocks it all. Find where the text for licensing him is and work from there
// Provides standardized 'point' to reference for memory
// Maybe this can be replaced with something else more reliable and something that stays the same no matter the game version?
static std::string signatureStr = "license with Bob";

// Gets the process ID of indycar
static DWORD FindProcessIdByWindow(const std::vector<std::wstring>& keywords)
{
    struct FindWindowData
    {
        std::vector<std::wstring> keywords;
        DWORD                     pid;
    } data{keywords, 0};

    EnumWindows([]
#if defined(__GNUC__) && !defined(__clang__)
                CALLBACK
#endif
                (HWND hwnd, LPARAM lParam) -> BOOL {
                    auto* wdata = reinterpret_cast<FindWindowData*>(lParam);
                    TCHAR title[256];
                    GetWindowText(hwnd, title, sizeof(title) / sizeof(TCHAR));
#if !defined(UNICODE)
                    std::wstring titleStr = ToLower(AnsiToWide(title));
#else
        const std::wstring titleStr = ToLower(title);
#endif
                    LogMessage(L"[DEBUG] Checking window \"" + titleStr + L"\"");
                    for (const auto& key : wdata->keywords)
                    {
                        auto query = ToLower(key);
                        if (titleStr.find(query) != std::wstring::npos)
                        {
                            LogMessage(L"[DEBUG] Window \"" + titleStr + L"\" matches \"" + key + L'\"');
                            GetWindowThreadProcessId(hwnd, &wdata->pid);
                            return FALSE;
                        }
                    }

                    return TRUE;
                },
                reinterpret_cast<LPARAM>(&data));

    return data.pid;
}

// Really don't understand this, but here is where we scan the memory for the data needed
static uintptr_t ScanSignature(HANDLE processHandle)
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    LogMessage(L"[DEBUG] Scanning for game...");
    LogMessage(L"[DEBUG] Process min addr: 0x" + std::to_wstring(reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress)));
    LogMessage(L"[DEBUG] Process max addr: 0x" + std::to_wstring(reinterpret_cast<uintptr_t>(sysInfo.lpMaximumApplicationAddress)));

    uintptr_t       addr    = reinterpret_cast<uintptr_t>(sysInfo.lpMinimumApplicationAddress);
    const uintptr_t maxAddr = 0x7FFFFFFF;

    MEMORY_BASIC_INFORMATION mbi;
    const size_t             targetLen = signatureStr.size();

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
                    for (SIZE_T i = 0; i <= bytesRead - targetLen; ++i)
                    {
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif
                        if (memcmp(buffer.data() + i, signatureStr.c_str(), targetLen) == 0)
                        {
#if defined(__clang__)
#    pragma clang diagnostic pop
#endif
                            std::wstringstream ss;
                            ss << L"[MATCH] Found Game at 0x" << std::hex << (addr + i);
                            LogMessage(ss.str());
                            return addr + i;
                        }
                    }
                }
            }
            addr += mbi.RegionSize;
        }
        else
            addr += 0x1000;
    }

    LogMessage(L"[ERROR] Signature not found in game.");
    return 0;
}

TelemetryReader::TelemetryReader(const FFBConfig& config) :
    offs(),
    out()
{
    if (config.targetGameWindowName.empty())
    {
        LogMessage(L"[ERROR] targetGameWindowName is not set.");
        return;
    }

    // Keywords to find game. "dosbox" + whatever is in the ini as "Game:"
    const std::vector<std::wstring> keywords = {L"dosbox", config.targetGameWindowName};
    const DWORD                     pid      = FindProcessIdByWindow(keywords);
    if (!pid) return;

    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) return;

    const uintptr_t sigAddr = ScanSignature(hProcess);
    if (!sigAddr)
    {
        CloseHandle(hProcess);
        hProcess = NULL;
        return;
    }

    offs = GetGameOffsets(config.version);
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
    return mInitialized && hProcess;
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
    out.rotation_deg = static_cast<double>(carData.data[7]) / 2147483648.0 /* static_cast<double>(INT_MAX) */ * 180.0;
    out.speed_mph    = static_cast<double>(carData.data[8]) / 75.0;
    out.steering_deg = static_cast<double>(carData.data[10]) / 11600000.0;
    out.steering_raw = static_cast<double>(carData.data[10]);
}

void TelemetryReader::ConvertTireData()
{
    // Tire data! Probably not loads, we dont know what it is

    out.tireload_lf   = static_cast<double>(rawData.loadLF);
    out.tireload_rf   = static_cast<double>(rawData.loadRF);
    out.tireload_lr   = static_cast<double>(rawData.loadLR);
    out.tireload_rr   = static_cast<double>(rawData.loadRR);
    out.tiremaglat_lf = static_cast<double>(rawData.magLatLF);
    out.tiremaglat_rf = static_cast<double>(rawData.magLatRF);
    out.tiremaglat_lr = static_cast<double>(rawData.magLatLR);
    out.tiremaglat_rr = static_cast<double>(rawData.magLatRR);
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

bool TelemetryReader::ReadLongitudinalForce()
{
    if (!ReadValue(rawData.longiF, offs.car_longitude_offset))
    {
        LogMessage(L"[ERROR] Failed to read longitude force");
        out.long_force = 0.0;
        return false;
    }
    else
    {
        out.long_force = static_cast<double>(rawData.longiF);
        return true;
    }
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
        ReadLongitudinalForce() &&
        ReadTireData();
    return out.valid;
}
