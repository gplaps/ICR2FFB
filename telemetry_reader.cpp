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
static DWORD carsDataAddr = 0;
static bool telemetryInitialized = false;

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
    DWORD tire_maglong_offsetfl;
    DWORD tire_maglong_offsetfr;
    DWORD tire_maglong_offsetrl;
    DWORD tire_maglong_offsetrr;
};

// Offsets database for different games/versions
// THANK YOU ERIC

// Rendition EXE
constexpr GameOffsets Offsets_REND = {
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
     0xEAB04, //lr tire long load
     0xEAB06, //rr tire long load
     0xEAB00, //lr tire long load
     0xEAB02, //rr tire long load
};

// DOS4G Exe, should be 1.02
constexpr GameOffsets Offsets_DOS = {
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
constexpr GameOffsets Offsets_WINDY = {
     0x004E2161,
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
     0x005281F6,
};

//N1 Offsets
constexpr GameOffsets Offsets_NASCAR = {
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

//N2 Offsets
constexpr GameOffsets Offsets_NASCAR2 = {
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

// Provides standardized 'point' to reference for memory
// Maybe this can be replaced with something else more reliable and something that stays the same no matter the game version?
//const char* signatureStr = "license with Bob";

// === Helpers ===

std::wstring ToLower(const std::wstring& str) {
    std::wstring result = str;
    for (wchar_t& ch : result) ch = towlower(ch);
    return result;
}

const char* GetSignatureString() {
    std::wstring gameName = ToLower(targetGameVersion);

    if (gameName == L"icr2dos" || gameName == L"icr2rend") {
        return "license with Bob";
    }
    else if (gameName == L"nascar1") {
        return "name of Harry Gant";
    }
    else if (gameName == L"nascar2") {
        return "NASCAR V2.03";
    }
    else {
        // Default fallback
        LogMessage(L"[DEBUG] Invalid Game Version " + targetGameVersion);
        return nullptr;
    }
}

// Helper function to check if a specific process window contains a keyword
bool CheckWindowForKeyword(DWORD pid, const std::wstring& keyword) {
    struct CheckWindowData {
        DWORD targetPid;
        std::wstring keyword;
        bool found = false;
    } data{ pid, ToLower(keyword), false };

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* data = reinterpret_cast<CheckWindowData*>(lParam);

        DWORD windowPid;
        GetWindowThreadProcessId(hwnd, &windowPid);

        if (windowPid == data->targetPid) {
            wchar_t title[256];
            GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t));
            std::wstring titleStr = ToLower(std::wstring(title));

            if (titleStr.find(data->keyword) != std::wstring::npos) {
                data->found = true;
                return FALSE; // Stop enumeration
            }
        }

        return TRUE; // Continue enumeration
        }, reinterpret_cast<LPARAM>(&data));

    return data.found;
}


// Gets the process ID of the window
DWORD FindProcessIdByWindow(const std::vector<std::wstring>& keywords) {
    struct FindWindowData {
        std::vector<std::wstring> keywords;
        DWORD pid = 0;
    } data{ keywords, 0 };

    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* data = reinterpret_cast<FindWindowData*>(lParam);
        wchar_t title[256];
        GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t));
        std::wstring titleStr = ToLower(std::wstring(title));

        for (const auto& key : data->keywords) {
            if (titleStr.find(ToLower(key)) == std::wstring::npos) return TRUE;
        }

        GetWindowThreadProcessId(hwnd, &data->pid);
        return FALSE;
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

    // Get the signature string based on game version
    const char* signatureStr = GetSignatureString();
    if (!signatureStr) {
        LogMessage(L"[ERROR] Could not get signature string for game version");
        return 0;
    }

    const size_t targetLen = strlen(signatureStr);

    while (addr < maxAddr) {
        if (VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
            if ((mbi.State == MEM_COMMIT) && !(mbi.Protect & PAGE_NOACCESS)) {
                std::vector<BYTE> buffer(mbi.RegionSize);
                SIZE_T bytesRead = 0;

                if (ReadProcessMemory(hProcess, (LPCVOID)addr, buffer.data(), mbi.RegionSize, &bytesRead)) {
                    for (SIZE_T i = 0; i <= bytesRead - targetLen; ++i) {
                        if (memcmp(buffer.data() + i, signatureStr, targetLen) == 0) {
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

/*
void SearchForMaxwellHouseCoffee(HANDLE hProcess) {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    const char* searchStr = "Maxwell House Coffee";
    const size_t searchLen = strlen(searchStr);

    LogMessage(L"[SEARCH] Starting memory search for 'Maxwell House Coffee'");
    LogMessage(L"[SEARCH] Process min addr: 0x" + std::to_wstring((uintptr_t)sysInfo.lpMinimumApplicationAddress));
    LogMessage(L"[SEARCH] Process max addr: 0x" + std::to_wstring((uintptr_t)sysInfo.lpMaximumApplicationAddress));

    uintptr_t addr = (uintptr_t)sysInfo.lpMinimumApplicationAddress;
    uintptr_t maxAddr = 0x7FFFFFFF;
    int foundCount = 0;

    while (addr < maxAddr) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi)) == sizeof(mbi)) {
            // Only search readable memory
            if ((mbi.State == MEM_COMMIT) && !(mbi.Protect & PAGE_NOACCESS) &&
                (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {

                std::vector<BYTE> buffer(mbi.RegionSize);
                SIZE_T bytesRead = 0;

                if (ReadProcessMemory(hProcess, (LPCVOID)addr, buffer.data(), mbi.RegionSize, &bytesRead)) {
                    // Search for the string in this memory block
                    for (SIZE_T i = 0; i <= bytesRead - searchLen; ++i) {
                        if (memcmp(buffer.data() + i, searchStr, searchLen) == 0) {
                            uintptr_t foundAddr = addr + i;
                            foundCount++;

                            std::wstringstream ss;
                            ss << L"[FOUND #" << foundCount << L"] Maxwell House Coffee at: 0x" << std::hex << foundAddr;
                            LogMessage(ss.str());

                            // Show surrounding context (32 bytes before and after)
                            if (i >= 32 && (i + searchLen + 32) <= bytesRead) {
                                std::wstring context = L"[CONTEXT] ";
                                for (int j = -32; j <= 32; j++) {
                                    BYTE byte = buffer[i + searchLen / 2 + j];
                                    if (byte >= 32 && byte <= 126) {  // Printable ASCII
                                        context += (wchar_t)byte;
                                    }
                                    else {
                                        context += L'.';
                                    }
                                }
                                LogMessage(context);
                            }
                        }
                    }
                }
            }
            addr += mbi.RegionSize;
        }
        else {
            addr += 0x1000;  // Skip to next page
        }
    }

    LogMessage(L"[SEARCH] Search complete. Found " + std::to_wstring(foundCount) + L" instances.");
}
*/
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
    static uintptr_t tireMagLongAddrLF = 0;
    static uintptr_t tireMagLongAddrFR = 0;
    static uintptr_t tireMagLongAddrLR = 0;
    static uintptr_t tireMagLongAddrRR = 0;


    SIZE_T bytesRead = 0;
    int16_t loadLF = 0, loadFR = 0, loadLR = 0, loadRR = 0;
    int16_t magLatLF = 0, magLatFR = 0, magLatLR = 0, magLatRR = 0;
    int16_t magLongLF = 0, magLongFR = 0, magLongLR = 0, magLongRR = 0;

    // Select game version
    std::wstring gameVersionLower = ToLower(targetGameVersion);
    const GameOffsets* offsets = nullptr;

    if (gameVersionLower == L"icr2dos") {
        offsets = &Offsets_DOS;
    }
    else if (gameVersionLower == L"icr2rend") {
        offsets = &Offsets_REND;
    }
    else if (gameVersionLower == L"nascar1") {
        offsets = &Offsets_NASCAR;
    }
    else if (gameVersionLower == L"nascar2") {
        offsets = &Offsets_NASCAR2;
    }
    else {
        LogMessage(L"[ERROR] Unknown game version: " + targetGameVersion);
        return false;
    }

    //LogMessage(L"[DEBUG] Offsets pointer: " + std::to_wstring((uintptr_t)offsets));
    //LogMessage(L"[DEBUG] Direct offset read: sig=0x" + std::to_wstring(offsets->signatureOffset) + L" car=0x" + std::to_wstring(offsets->cars_data_offset));

    if (!hProcess) {
        if (targetGameVersion.empty()) {
            LogMessage(L"[ERROR] Game Version is not set.");
            return false;
        }

        // Keywords to find game based on game version
        std::vector<std::wstring> keywords;
        DWORD pid = 0;

        //game versions for icr2
        if (gameVersionLower == L"icr2dos" || gameVersionLower == L"icr2rend") {
            // Try indycar first
            keywords = { L"dosbox", L"indycar" };
            pid = FindProcessIdByWindow(keywords);

            if (!pid) {
                // Fallback to cart keywords
                keywords = { L"dosbox", L"cart" };
                pid = FindProcessIdByWindow(keywords);

                if (!pid) {
                    LogMessage(L"[ERROR] Game window not found!");
                    LogMessage(L"[ERROR] Looking for window containing: 'dosbox' AND ('indycar' OR 'cart')");
                    LogMessage(L"[ERROR] Make sure " + targetGameVersion + L" is running in DOSBox");
                    return false;
                }
                else {
                    LogMessage(L"[INFO] Found game window with 'cart' keywords");
                }
            }
            else {
                LogMessage(L"[INFO] Found game window with 'indycar' keywords");
            }

            // Version verification code (your existing logic)
            bool windowHasRendition = CheckWindowForKeyword(pid, L"rendition");

            if (gameVersionLower == L"icr2dos" && windowHasRendition) {
                LogMessage(L"[ERROR] Wrong game version detected!");
                LogMessage(L"[ERROR] Found Rendition version but config is set to 'ICR2DOS'");
                LogMessage(L"[ERROR] Please change your INI file to use 'ICR2REND' instead");
                return false;
            }
            else if (gameVersionLower == L"icr2rend" && !windowHasRendition) {
                LogMessage(L"[ERROR] Wrong game version detected!");
                LogMessage(L"[ERROR] Found DOS version but config is set to 'ICR2REND'");
                LogMessage(L"[ERROR] Please change your INI file to use 'ICR2DOS' instead");
                return false;
            }
            else {
                LogMessage(L"[INFO] Correct game version detected and verified");
            }
        }

        //game versions for N1
        else if (gameVersionLower == L"nascar1") {
            keywords = { L"dosbox", L"nascar" };
            pid = FindProcessIdByWindow(keywords);
            if (!pid) {
                LogMessage(L"[ERROR] NASCAR game window not found!");
                LogMessage(L"[ERROR] Looking for window containing: 'dosbox' AND 'nascar'");
                LogMessage(L"[ERROR] Make sure NASCAR Racing is running in DOSBox");
                return false;
            }
            else {
                LogMessage(L"[INFO] Found NASCAR game window");
            }
        }

        //game versions for N2
        else if (gameVersionLower == L"nascar2") {
            keywords = { L"dosbox", L"nascar2" };
            pid = FindProcessIdByWindow(keywords);
            if (!pid) {
                LogMessage(L"[ERROR] NASCAR 2 game window not found!");
                LogMessage(L"[ERROR] Looking for window containing: 'dosbox' AND 'nascar2'");
                LogMessage(L"[ERROR] Make sure NASCAR Racing 2 is running in DOSBox");
                return false;
            }
            else {
                LogMessage(L"[INFO] Found NASCAR 2 game window");
            }
        }

        //LogMessage(L"[DEBUG] Raw targetGameVersion: '" + targetGameVersion + L"'");
        //LogMessage(L"[DEBUG] gameVersionLower: '" + gameVersionLower + L"'");

        hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!hProcess) return false;

        //SearchForMaxwellHouseCoffee(hProcess);

        uintptr_t sigAddr = ScanSignature(hProcess);
        if (!sigAddr) {
            CloseHandle(hProcess);
            hProcess = nullptr;
            return false;
        }

        uintptr_t exeBase = sigAddr - offsets->signatureOffset;
        carsDataAddr = exeBase + offsets->cars_data_offset;
        tireLoadAddrLF = exeBase + offsets->tire_data_offsetfl;
        tireLoadAddrFR = exeBase + offsets->tire_data_offsetfr;
        tireLoadAddrLR = exeBase + offsets->tire_data_offsetrl;
        tireLoadAddrRR = exeBase + offsets->tire_data_offsetrr;
        tireMagLatAddrLF = exeBase + offsets->tire_maglat_offsetfl;
        tireMagLatAddrFR = exeBase + offsets->tire_maglat_offsetfr;
        tireMagLatAddrLR = exeBase + offsets->tire_maglat_offsetrl;
        tireMagLatAddrRR = exeBase + offsets->tire_maglat_offsetrr;
        tireMagLongAddrLF = exeBase + offsets->tire_maglong_offsetfl;
        tireMagLongAddrFR = exeBase + offsets->tire_maglong_offsetfr;
        tireMagLongAddrLR = exeBase + offsets->tire_maglong_offsetrl;
        tireMagLongAddrRR = exeBase + offsets->tire_maglong_offsetrr;

        /*
        if (gameVersionLower == L"icr2dos" || gameVersionLower == L"icr2rend") {

            LogMessage(L"=== ICR2 MEMORY ANALYSIS ===");

            // Log the found signature address
            LogMessage(L"[ICR2] Signature found at: 0x" + std::to_wstring(sigAddr));

            // Log the signature offset being used
            LogMessage(L"[ICR2] Using signature offset: 0x" + std::to_wstring(offsets->signatureOffset));

            // Calculate and log the EXE base
            uintptr_t calculatedExeBase = sigAddr - offsets->signatureOffset;
            LogMessage(L"[ICR2] Calculated EXE base: 0x" + std::to_wstring(calculatedExeBase));

            // Known values from your analysis
            uintptr_t knownFileOffset = 0xF21CC;  // From HxD
            uintptr_t knownCheatEngineAddr = 0x1054BD98;  // From Cheat Engine
            uintptr_t knownSignatureOffset = 0xA0D78;  // From working code

            LogMessage(L"[ICR2] Known file offset: 0x" + std::to_wstring(knownFileOffset));
            LogMessage(L"[ICR2] Known Cheat Engine addr: 0x" + std::to_wstring(knownCheatEngineAddr));
            LogMessage(L"[ICR2] Known signature offset: 0x" + std::to_wstring(knownSignatureOffset));

            // Calculate various relationships
            uintptr_t memoryToFileOffset = sigAddr - knownFileOffset;
            LogMessage(L"[ICR2] Memory to file offset diff: 0x" + std::to_wstring(memoryToFileOffset));

            uintptr_t baseFromFile = knownCheatEngineAddr - knownFileOffset;
            LogMessage(L"[ICR2] Base calculated from file: 0x" + std::to_wstring(baseFromFile));

            uintptr_t baseFromSignature = knownCheatEngineAddr - knownSignatureOffset;
            LogMessage(L"[ICR2] Base calculated from signature: 0x" + std::to_wstring(baseFromSignature));

            // Test if our current calculation matches the working method
            bool calculationMatches = (calculatedExeBase == baseFromSignature);
            LogMessage(L"[ICR2] Current calculation matches working method: " + std::wstring(calculationMatches ? L"YES" : L"NO"));

            // Show the actual working car data address
            uintptr_t workingCarDataAddr = calculatedExeBase + offsets->cars_data_offset;
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
            uintptr_t nascarSigOffset3 = nascarMemoryAddr - (calculatedExeBase - sigAddr + nascarMemoryAddr);
            LogMessage(L"[NASCAR CALC 3] Direct calculation: Need to determine correct base");

            LogMessage(L"=== END ICR2 ANALYSIS ===");
          */
          //}

         // LogMessage(L"[DEBUG] Selected signatureOffset: 0x" + std::to_wstring(offsets->signatureOffset));
         // LogMessage(L"[DEBUG] Selected cars_data_offset: 0x" + std::to_wstring(offsets->cars_data_offset));
         // LogMessage(L"[DEBUG] Raw calculation: 0x" + std::to_wstring(sigAddr) + L" - 0x" + std::to_wstring(offsets->signatureOffset) + L" + 0x" + std::to_wstring(offsets->cars_data_offset));

          //temp debug
         // LogMessage(L"[DEBUG] Signature found at: 0x" + std::to_wstring(sigAddr));
         // LogMessage(L"[DEBUG] Calculated EXE base: 0x" + std::to_wstring(exeBase));
         // LogMessage(L"[DEBUG] Calculated car data addr: 0x" + std::to_wstring(carsDataAddr));
         // LogMessage(L"[DEBUG] Expected car data addr: 0x1058474C");

        LogMessage(L"[INIT] EXE base: 0x" + std::to_wstring(exeBase) +
            L" | cars_data @ 0x" + std::to_wstring(carsDataAddr) + 
            L" | rf_mag_lat @ 0x" + std::to_wstring(tireMagLatAddrFR));
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

    //LogMessage(L"[DEBUG] car0_data address: 0x" + std::to_wstring(carsDataAddr));

 
    bool tireOK =
        ReadProcessMemory(hProcess, (LPCVOID)tireLoadAddrLF, &loadLF, sizeof(loadLF), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireLoadAddrFR, &loadFR, sizeof(loadFR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireLoadAddrLR, &loadLR, sizeof(loadLR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireLoadAddrRR, &loadRR, sizeof(loadRR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLatAddrLF, &magLatLF, sizeof(magLatLF), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLatAddrFR, &magLatFR, sizeof(magLatFR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLatAddrLR, &magLatLR, sizeof(magLatLR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLatAddrRR, &magLatRR, sizeof(magLatRR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLongAddrLF, &magLongLF, sizeof(magLongLF), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLongAddrFR, &magLongFR, sizeof(magLongFR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLongAddrLR, &magLongLR, sizeof(magLongLR), &bytesRead) &&
        ReadProcessMemory(hProcess, (LPCVOID)tireMagLongAddrRR, &magLongRR, sizeof(magLongRR), &bytesRead);
  
    //bool tireOK = true;

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
    out.tiremaglong_lf = static_cast<double>(magLongLF);
    out.tiremaglong_rf = static_cast<double>(magLongFR);
    out.tiremaglong_lr = static_cast<double>(magLongLR);
    out.tiremaglong_rr = static_cast<double>(magLongRR);
    out.valid = true;

    return true;
}