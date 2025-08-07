#include "ffb_setup.h"
#include <fstream>
#include <iostream>
#include "constant_force.h"

extern IDirectInputEffect* constantForceEffect;

//settings from the ffb.ini
std::wstring targetDeviceName;
std::wstring targetGameVersion;
std::wstring targetGameWindowName;
std::wstring targetForceSetting;
std::wstring targetDamperEnabled;
std::wstring targetSpringEnabled;
std::wstring targetConstantEnabled;
std::wstring targetInvertFFB;

IDirectInputDevice8* matchedDevice = nullptr;
LPDIRECTINPUT8 directInput = nullptr;

BOOL CALLBACK EnumDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID*) {
    std::wstring deviceName = pdidInstance->tszProductName;
    if (deviceName == targetDeviceName) {
        LogMessage(L"[INFO] Matched Device: " + deviceName);
        HRESULT hr = directInput->CreateDevice(pdidInstance->guidInstance, &matchedDevice, nullptr);
        return (FAILED(hr)) ? DIENUM_CONTINUE : DIENUM_STOP;
    }
    return DIENUM_CONTINUE;
}

// Search the ini file for settings and find what the user has set them to
bool LoadFFBSettings(const std::wstring& filename) {
    std::wifstream file(filename);
    if (!file) return false;
    std::wstring line;
    while (std::getline(file, line)) {
        if (line.rfind(L"Device: ", 0) == 0)
            targetDeviceName = line.substr(8);
        else if (line.rfind(L"Version: ", 0) == 0)
            targetGameVersion = line.substr(9);
        else if (line.rfind(L"Force: ", 0) == 0)
            targetForceSetting = line.substr(7);
        else if (line.rfind(L"Game: ", 0) == 0)
            targetGameWindowName = line.substr(6);
        else if (line.rfind(L"Damper: ", 0) == 0)
            targetDamperEnabled = line.substr(8);
        else if (line.rfind(L"Spring: ", 0) == 0)
            targetSpringEnabled = line.substr(8);
        else if (line.rfind(L"Constant: ", 0) == 0)
            targetConstantEnabled = line.substr(10);
        else if (line.rfind(L"Invert: ", 0) == 0)
            targetInvertFFB = line.substr(8);
    }
    return !targetDeviceName.empty();
}

// Kick-off DirectInput
bool InitializeDevice() {
    HRESULT hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&directInput, NULL);
    if (FAILED(hr)) return false;
    hr = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, NULL, DIEDFL_ATTACHEDONLY);
    return matchedDevice != nullptr;
}