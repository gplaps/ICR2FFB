#include "ffb_setup.h"
#include <fstream>
#include <iostream>
#include "constant_force.h"

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

extern IDirectInputEffect* constantForceEffect;

//settings from the ffb.ini
std::wstring targetDeviceName;
std::wstring targetGameVersion;
std::wstring targetGameWindowName;
std::wstring targetForceSetting;
std::wstring targetDeadzoneSetting;
std::wstring targetInvertFFB;
std::wstring targetLimitEnabled;
std::wstring targetConstantEnabled;
std::wstring targetConstantScale;
std::wstring targetWeightEnabled;
std::wstring targetWeightScale;
std::wstring targetDamperEnabled;
std::wstring targetDamperScale;
std::wstring targetSpringEnabled;



IDirectInputDevice8* matchedDevice = nullptr;
LPDIRECTINPUT8 directInput = nullptr;

// Device lists for better error messages
BOOL CALLBACK ListDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID*) {
#if !defined(UNICODE)
    std::wstring deviceName = ansiToWide(pdidInstance->tszProductName);
#else
    std::wstring deviceName = pdidInstance->tszProductName;
#endif
    LogMessage(L"[INFO] Available device: " + deviceName);
    return DIENUM_CONTINUE;  // Continue enumerating all devices
}

void ListAvailableDevices() {
    if (directInput) {
        LogMessage(L"[INFO] Enumerating available game controllers:");
        HRESULT hr = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, ListDevicesCallback, NULL, DIEDFL_ATTACHEDONLY);
        if (FAILED(hr)) {
            LogMessage(L"[ERROR] Failed to enumerate devices: 0x" + std::to_wstring(hr));
        }
        else {
            LogMessage(L"[INFO] Device enumeration complete");
        }
    }
    else {
        LogMessage(L"[ERROR] DirectInput not initialized, cannot list devices");
    }
}

BOOL CALLBACK ConsoleListDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID*) {
#if !defined(UNICODE)
    std::wstring deviceName = ansiToWide(pdidInstance->tszProductName);
#else
    std::wstring deviceName = pdidInstance->tszProductName;
#endif
    std::wcout << L"  - " << deviceName << std::endl;
    return DIENUM_CONTINUE;
}

void ShowAvailableDevicesOnConsole() {
    if (directInput) {
        directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, ConsoleListDevicesCallback, NULL, DIEDFL_ATTACHEDONLY);
    }
    else {
        std::wcout << L"  (Could not enumerate devices - DirectInput not initialized)" << std::endl;
    }
}


BOOL CALLBACK EnumDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID*) {
#if !defined(UNICODE)
    std::wstring deviceName = ansiToWide(pdidInstance->tszProductName);
#else
    std::wstring deviceName = pdidInstance->tszProductName;
#endif

    if (deviceName == targetDeviceName) {
        LogMessage(L"[INFO] Found matching device: " + deviceName);
        HRESULT hr = directInput->CreateDevice(pdidInstance->guidInstance, &matchedDevice, nullptr);
        if (FAILED(hr)) {
            LogMessage(L"[ERROR] Failed to create device: 0x" + std::to_wstring(hr));
            return DIENUM_CONTINUE;
        }
        LogMessage(L"[INFO] Successfully created device interface");
        return DIENUM_STOP;  // Found and created successfully
    }

    return DIENUM_CONTINUE;
}

// Search the ini file for settings and find what the user has set them to
bool LoadFFBSettings(const std::wstring& filename) {
    
    targetWeightEnabled = L"false";
    targetWeightScale = L"1.0";
    
    std::wifstream file(filename.c_str());
    if (!file) return false;
    std::wstring line;
    while (std::getline(file, line)) {
        if (line.rfind(L"Device: ", 0) == 0)
            targetDeviceName = line.substr(8);
        else if (line.rfind(L"Game: ", 0) == 0)
            targetGameWindowName = line.substr(6);
        else if (line.rfind(L"Version: ", 0) == 0)
            targetGameVersion = line.substr(9);
        else if (line.rfind(L"Force: ", 0) == 0)
            targetForceSetting = line.substr(7);
        else if (line.rfind(L"Deadzone: ", 0) == 0)
            targetDeadzoneSetting = line.substr(10);
        else if (line.rfind(L"Invert: ", 0) == 0)
            targetInvertFFB = line.substr(8);
        else if (line.rfind(L"Limit: ", 0) == 0)
            targetLimitEnabled = line.substr(7);
        else if (line.rfind(L"Constant: ", 0) == 0)
            targetConstantEnabled = line.substr(10);
        else if (line.rfind(L"Constant Scale: ", 0) == 0)
            targetConstantScale = line.substr(16);
        else if (line.rfind(L"Weight: ", 0) == 0)
            targetWeightEnabled = line.substr(8);
        else if (line.rfind(L"Weight Scale: ", 0) == 0)
            targetWeightScale = line.substr(14);
        else if (line.rfind(L"Damper: ", 0) == 0)
            targetDamperEnabled = line.substr(8);
        else if (line.rfind(L"Damper Scale: ", 0) == 0)
            targetDamperScale = line.substr(14);
        else if (line.rfind(L"Spring: ", 0) == 0)
            targetSpringEnabled = line.substr(8);


    }
    return !targetDeviceName.empty();
}

// Kick-off DirectInput
bool InitializeDevice() {
    LogMessage(L"[INFO] Initializing DirectInput...");

    HRESULT hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&directInput, NULL);
    if (FAILED(hr)) {
        LogMessage(L"[ERROR] DirectInput8Create failed: 0x" + std::to_wstring(hr));
        return false;
    }

    LogMessage(L"[INFO] Searching for device: " + targetDeviceName);
    hr = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, NULL, DIEDFL_ATTACHEDONLY);

    if (FAILED(hr)) {
        LogMessage(L"[ERROR] EnumDevices failed: 0x" + std::to_wstring(hr));
        return false;
    }

    if (matchedDevice == nullptr) {
        LogMessage(L"[ERROR] Device not found: " + targetDeviceName);
        return false;
    }

    LogMessage(L"[INFO] Device initialization successful");
    return true;
}