// telemetry_reader.cpp
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstdint>
#include <sstream>
#include <cwctype>

#include "telemetry_reader.h"
#include "ffb_setup.h"
#include "helpers.h"

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

// === Globals ===
static HANDLE hProcess = nullptr;
// static DWORD carsDataAddr = 0;
// static bool telemetryInitialized = false;

// Things to look for in the Memory to make it tick
struct GameOffsets {
    uintptr_t signatureOffset;
    DWORD cars_data_offset;
    DWORD tire_data_offsetfl;
    DWORD tire_data_offsetfr;
    DWORD tire_data_offsetrl;
    DWORD tire_data_offsetrr;
    DWORD tire_maglat_offsetfl;
    DWORD tire_maglat_offsetfr;
    DWORD tire_maglat_offsetrl;
    DWORD tire_maglat_offsetrr;
    DWORD car_longitude_offset;
};

// Offsets for different version of the game

// Rendition EXE
constexpr GameOffsets Offsets_REND = {
     0xB1C0C, 0xE0EA4, 0xBB4E8, 0xBB4EA, 0xBB4E4, 0xBB4E6, 0xEAB24, 0xEAB26, 0xEAB20, 0xEAB22, 0xEAB00
    //0xB1C0C, 0xE0EA4, 0xBB4E8, 0xBB4EA, 0xBB4E4, 0xBB4E6, 0xEAB16, 0xEAB14, 0xEAB12, 0xEAB10 // original maglat
};

// DOS4G Exe, should be 1.02
constexpr GameOffsets Offsets_DOS = {
    0xA0D78, 0xD4718, 0xA85B8, 0xA85BA, 0xA85B4, 0xA85B6, 0xC5C48, 0xC5C4A, 0xC5C44, 0xC5C46, 0xC5C14
    //0xA0D78, 0xD4718, 0xA85B8, 0xA85BA, 0xA85B4, 0xA85B6, 0xC5C2A, 0xC5C28, 0xC5C26, 0xC5C24 // original maglat
};

// BOB! Bobby Rahal unlocks it all. Find where the text for licensing him is and work from there
// Provides standardized 'point' to reference for memory
// Maybe this can be replaced with something else more reliable and something that stays the same no matter the game version?
const char* signatureStr = "license with Bob";

// Gets the process ID of indycar
DWORD FindProcessIdByWindow(const std::vector<std::wstring>& keywords) {
    struct FindWindowData {
        std::vector<std::wstring> keywords;
        DWORD pid = 0;
    } data{ keywords, 0 };

    EnumWindows([]
#if defined(__GNUC__) && !defined(__clang__)
        CALLBACK
#endif
        (HWND hwnd, LPARAM lParam) -> BOOL {
        auto* data = reinterpret_cast<FindWindowData*>(lParam);
        TCHAR title[256];
        GetWindowText(hwnd, title, sizeof(title) / sizeof(TCHAR));
#if !defined(UNICODE)
        std::wstring titleStr = ToLower(ansiToWide(title));
#else
        std::wstring titleStr = ToLower(title);
#endif
        for (const auto& key : data->keywords) {
            auto query = ToLower(key);
            if (titleStr.find(query) != std::wstring::npos) 
            {
                GetWindowThreadProcessId(hwnd, &data->pid);
                return FALSE;
            }
        }

        return TRUE;
        }, reinterpret_cast<LPARAM>(&data));

    return data.pid;
}

// Really don't understand this, but here is where we scan the memory for the data needed
uintptr_t ScanSignature(HANDLE hProcess) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    LogMessage(L"[DEBUG] Scanning for game...");
    LogMessage(L"[DEBUG] Process min addr: 0x" + std::to_wstring((uintptr_t)sysInfo.lpMinimumApplicationAddress));
    LogMessage(L"[DEBUG] Process max addr: 0x" + std::to_wstring((uintptr_t)sysInfo.lpMaximumApplicationAddress));

    uintptr_t addr = (uintptr_t)sysInfo.lpMinimumApplicationAddress;
    uintptr_t maxAddr = 0x7FFFFFFF;

    MEMORY_BASIC_INFORMATION mbi;
    const size_t targetLen = strlen(signatureStr);

    while (addr < maxAddr) {
        if (VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
            if ((mbi.State == MEM_COMMIT) && !(mbi.Protect & PAGE_NOACCESS)) {
                std::vector<BYTE> buffer(mbi.RegionSize);
                SIZE_T bytesRead = 0;

                if (ReadProcessMemory(hProcess, (LPCVOID)addr, buffer.data(), mbi.RegionSize, &bytesRead)) {
                    for (SIZE_T i = 0; i <= bytesRead - targetLen; ++i) {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif
                        if (memcmp(buffer.data() + i, signatureStr.c_str(), targetLen) == 0) {
#if defined(__clang__)
#pragma clang diagnostic pop
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
        else {
            addr += 0x1000;
        }
    }

    LogMessage(L"[ERROR] Signature not found in game.");
    return 0;
}

// === Main ===

bool ReadTelemetryData(RawTelemetry& out) {
    static uintptr_t carsDataAddr = 0;
    static uintptr_t tireLoadAddrLF = 0;
    static uintptr_t tireLoadAddrFR = 0;
    static uintptr_t tireLoadAddrLR = 0;
    static uintptr_t tireLoadAddrRR = 0;
    static uintptr_t tireMagLatAddrLF = 0;
    static uintptr_t tireMagLatAddrFR = 0;
    static uintptr_t tireMagLatAddrLR = 0;
    static uintptr_t tireMagLatAddrRR = 0;
    static uintptr_t carLongitudeAddr = 0;


    SIZE_T bytesRead = 0;
    int16_t loadLF = 0, loadFR = 0, loadLR = 0, loadRR = 0;
    int16_t magLatLF = 0, magLatFR = 0, magLatLR = 0, magLatRR = 0;
    int16_t longiF = 0;

    // Select between Dos and Rendition version. Rendition is default
    const GameOffsets& offsets = (ToLower(targetGameVersion) == L"dos4g") ? Offsets_DOS : Offsets_REND;

    if (!hProcess) {
        if (targetGameWindowName.empty()) {
            LogMessage(L"[ERROR] targetGameWindowName is not set.");
            return false;
        }

        // Keywords to find game. "dosbox" + whatever is in the ini as "Game:"
        std::vector<std::wstring> keywords = { L"dosbox", targetGameWindowName };
        DWORD pid = FindProcessIdByWindow(keywords);
        if (!pid) return false;

        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!hProcess) return false;

        uintptr_t sigAddr = ScanSignature(hProcess);
        if (!sigAddr) {
            CloseHandle(hProcess);
            hProcess = nullptr;
            return false;
        }

        uintptr_t exeBase = sigAddr - offsets.signatureOffset;
        carsDataAddr = exeBase + offsets.cars_data_offset;
        tireLoadAddrLF = exeBase + offsets.tire_data_offsetfl;
        tireLoadAddrFR = exeBase + offsets.tire_data_offsetfr;
        tireLoadAddrLR = exeBase + offsets.tire_data_offsetrl;
        tireLoadAddrRR = exeBase + offsets.tire_data_offsetrr;
        tireMagLatAddrLF = exeBase + offsets.tire_maglat_offsetfl;
        tireMagLatAddrFR = exeBase + offsets.tire_maglat_offsetfr;
        tireMagLatAddrLR = exeBase + offsets.tire_maglat_offsetrl;
        tireMagLatAddrRR = exeBase + offsets.tire_maglat_offsetrr;
        carLongitudeAddr = exeBase + offsets.car_longitude_offset;

        LogMessage(L"[INIT] EXE base: 0x" + std::to_wstring(exeBase) +
            L" | cars_data @ 0x" + std::to_wstring(carsDataAddr));
    }

    int32_t car0_data[12] = { 0 };
    if (!ReadProcessMemory(hProcess, (LPCVOID)carsDataAddr, &car0_data, sizeof(car0_data), &bytesRead)) {
        LogMessage(L"[ERROR] Failed to read car0 data. GetLastError(): " + std::to_wstring(GetLastError()));
        CloseHandle(hProcess);
        hProcess = nullptr;
        return false;
    }

    // Set variables to be used everywhere else
    // Little bit of math to make the data sensible. Save big calculations for specific "Calculation" sets
    out.dlong = static_cast<double>(car0_data[4]);
    out.dlat = static_cast<double>(car0_data[5]);
    out.rotation_deg = static_cast<double>(car0_data[7]) / 2147483648.0 * 180.0;
    out.speed_mph = static_cast<double>(car0_data[8]) / 75.0;
    out.steering_deg = static_cast<double>(car0_data[10]) / 11600000.0;
    out.steering_raw = static_cast<double>(car0_data[10]);

    ReadProcessMemory(hProcess, (LPCVOID)carLongitudeAddr, &longiF, sizeof(longiF), &bytesRead);
    if (bytesRead != sizeof(longiF)) {
        LogMessage(L"[ERROR] Failed to read longitude force. Bytes read: " + std::to_wstring(bytesRead));
        out.long_force = 0.0;
    }
    else {
        out.long_force = static_cast<double>(longiF);
    }

    bool tireOK =
        ReadProcessMemory(hProcess, (LPCVOID)tireLoadAddrLF, &loadLF, sizeof(loadLF), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireLoadAddrFR, &loadFR, sizeof(loadFR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireLoadAddrLR, &loadLR, sizeof(loadLR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireLoadAddrRR, &loadRR, sizeof(loadRR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLatAddrLF, &magLatLF, sizeof(magLatLF), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLatAddrFR, &magLatFR, sizeof(magLatFR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLatAddrLR, &magLatLR, sizeof(magLatLR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLatAddrRR, &magLatRR, sizeof(magLatRR), &bytesRead);

    if (!tireOK) {
        LogMessage(L"[ERROR] Failed to read one or more tire loads.");
        return false;
    }

    // Tire data! Probably not loads, we dont know what it is

    out.tireload_lf = static_cast<double>(loadLF);
    out.tireload_rf = static_cast<double>(loadFR);
    out.tireload_lr = static_cast<double>(loadLR);
    out.tireload_rr = static_cast<double>(loadRR);
    out.tiremaglat_lf = static_cast<double>(magLatLF);
    out.tiremaglat_rf = static_cast<double>(magLatFR);
    out.tiremaglat_lr = static_cast<double>(magLatLR);
    out.tiremaglat_rr = static_cast<double>(magLatRR);
    out.valid = true;

    return true;
}