#include "telemetry_reader.h"

#include "ffb_config.h"
#include "game_detect.h"
#include "game_version.h"
#include "log.h"
#include "string_utilities.h" // IWYU pragma: keep

#include <psapi.h>
#if !defined(IS_CPP11_COMPLIANT)
#    include <stdint.h>
#endif
#include <tlhelp32.h>

#include <cwctype>

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

TelemetryReader::TelemetryReader(FFBConfig& config) :
    hProcess(NULL),
    mInitialized(false),
    offsets(),
    out(),
    rawData(),
    carData()
{
    if (!Initialize(config))
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

bool TelemetryReader::Initialize(FFBConfig& config)
{
    if (mInitialized) { return true; }

    // Find process window
    const DWORD pid = FindProcessIdByWindow(config.GetString(L"other", L"window"));
    if (!pid)
    {
        LogMessage(L"[ERROR] Game window not found.");
        return false;
    }

    hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) { return false; }

    // scan for supported games
    config.game = ScanSignature(hProcess);
    if (!config.game.Valid())
    {
        CloseHandle(hProcess);
        hProcess = NULL;
        return false;
    }

    offsets = config.game.Offsets();

    LogMessage(L"[INIT] EXE base: 0x" + std::to_wstring(offsets.signature) +
               L" | cars_data @ 0x" + std::to_wstring(offsets.cars_data) +
               L" | rf_mag_lat @ 0x" + std::to_wstring(offsets.tire_maglat_fr));

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
    out.rotation_raw     = static_cast<double>(carData.data[7]);

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
