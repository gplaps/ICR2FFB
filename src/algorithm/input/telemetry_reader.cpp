#include "telemetry_reader.h"

#include "ffb_config.h"
#include "game_detect.h"
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

TelemetryReader::TelemetryReader(const FFBConfig& config) :
    hProcess(NULL),
    mInitialized(false),
    offsets(),
    out(),
    rawData(),
    carData()
{
    if(!Initialize())
    {
        LogMessage(L"[ERROR] TelemetryReader failed to initialize.");
        return;
    }
}

TelemetryReader::~TelemetryReader()
{
    if (hProcess)
    {
        CloseHandle(hProcess);
        hProcess = NULL;
    }
}

bool TelemetryReader::Initialize(const FFBConfig& config)
{
    if(mInitialized) { return true; }

    if (config.GetString(L"base", L"game").empty())
    {
        LogMessage(L"[ERROR] \"Game\" is not set.");
        return false;
    }

    // Find process window
    const DWORD pid = FindProcessIdByWindow(config.version);
    if (!pid)
    {
        LogMessage(L"[ERROR] Game window not found.");
        return;
    }

    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) { return false; }

    // it should be possible to detect any supported game by searching the memory for known signatures and selecting one of the known offsets for it without the need for the user to specify it
    // if more game offsets have been found this project likely has to undergo a (significant) rewrite
    offsets                 = GetGameOffsets(config.version);
    const uintptr_t signatureAddress = ScanSignature(hProcess, offsets);
    if (!signatureAddress)
    {
        CloseHandle(hProcess);
        hProcess = NULL;
        return false;
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
    return mInitialized;
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
