#include "direct_input.h"
#include "log.h"
#include <iostream>

LPDIRECTINPUT8 DirectInput::directInput = NULL;

static std::wstring targetDeviceNameLocal; // only needed because of function signature of callback ... should be avoided if it can be expressed differently

static BOOL CALLBACK ConsoleListDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID*) {
    #if !defined(UNICODE)
        std::wstring deviceName = ansiToWide(pdidInstance->tszProductName);
    #else
        std::wstring deviceName = pdidInstance->tszProductName;
    #endif
        std::wcout << L"  - " << deviceName << std::endl;
        return DIENUM_CONTINUE;
    }
    
static BOOL CALLBACK EnumDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* content) {
    #if !defined(UNICODE)
            std::wstring deviceName = ansiToWide(pdidInstance->tszProductName);
    #else
            std::wstring deviceName = pdidInstance->tszProductName;
    #endif
        
            if (deviceName == targetDeviceNameLocal && content != nullptr) {
                FFBDevice* device = (FFBDevice*)content;
                LogMessage(L"[INFO] Found matching device: " + deviceName);
                HRESULT hr = DirectInput::directInput->CreateDevice(pdidInstance->guidInstance, &device->matchedDevice, NULL);
                if (FAILED(hr)) {
                    LogMessage(L"[ERROR] Failed to create device: 0x" + std::to_wstring(hr));
                    return DIENUM_CONTINUE;
                }
                LogMessage(L"[INFO] Successfully created device interface");
                return DIENUM_STOP;  // Found and created successfully
            }
        
            return DIENUM_CONTINUE;
        }
    
// Device lists for better error messages
static BOOL CALLBACK ListDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID*) {
#if !defined(UNICODE)
    std::wstring deviceName = ansiToWide(pdidInstance->tszProductName);
#else
    std::wstring deviceName = pdidInstance->tszProductName;
#endif
    LogMessage(L"[INFO] Available device: " + deviceName);
    return DIENUM_CONTINUE;  // Continue enumerating all devices
}

void DirectInput::ListAvailableDevices() {
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

void DirectInput::ShowAvailableDevicesOnConsole() {
    if (directInput) {
        directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, ConsoleListDevicesCallback, NULL, DIEDFL_ATTACHEDONLY);
    }
    else {
        std::wcout << L"  (Could not enumerate devices - DirectInput not initialized)" << std::endl;
    }
}

// Kick-off DirectInput
bool DirectInput::InitializeDevice(FFBDevice& device) {
    LogMessage(L"[INFO] Initializing DirectInput...");

    HRESULT hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&directInput, NULL);
    if (FAILED(hr)) {
        LogMessage(L"[ERROR] DirectInput8Create failed: 0x" + std::to_wstring(hr));
        return false;
    }

    LogMessage(L"[INFO] Searching for device: " + device.config.targetDeviceName);
    targetDeviceNameLocal = device.config.targetDeviceName;
    hr = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, &device, DIEDFL_ATTACHEDONLY);

    if (FAILED(hr)) {
        LogMessage(L"[ERROR] EnumDevices failed: 0x" + std::to_wstring(hr));
        return false;
    }

    if (device.matchedDevice == NULL) {
        LogMessage(L"[ERROR] Device not found: " + device.config.targetDeviceName);
        return false;
    }

    LogMessage(L"[INFO] Device initialization successful");
    return true;
}
