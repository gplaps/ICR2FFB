#include "direct_input.h"

#include "helpers.h"
#include "log.h"

#include <iostream>
#include <string>

LPDIRECTINPUT8 DirectInput::directInput = NULL;

struct EnumDeviceHelper
{
    EnumDeviceHelper(FFBDevice* d, const std::wstring& name) :
        device(d),
        targetDeviceName(name) {}
    FFBDevice*   device;
    std::wstring targetDeviceName;
};

static BOOL CALLBACK ConsoleListDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID*)
{
    const std::wstring deviceName = ToWideString(pdidInstance->tszProductName);
    std::wcout << L"  - " << deviceName << L'\n';
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK EnumDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* content)
{
    if (!content) { return DIENUM_CONTINUE; }

    const std::wstring deviceName = ToWideString(pdidInstance->tszProductName);

    EnumDeviceHelper* edh         = static_cast<EnumDeviceHelper*>(content);
    if (deviceName == edh->targetDeviceName)
    {
        LogMessage(L"[INFO] Found matching device: " + deviceName);
        const HRESULT hr = DirectInput::directInput->CreateDevice(pdidInstance->guidInstance, &edh->device->diDevice, NULL);
        if (FAILED(hr))
        {
            LogMessage(L"[ERROR] Failed to create device: 0x" + std::to_wstring(hr));
            return DIENUM_CONTINUE;
        }
        LogMessage(L"[INFO] Successfully created device interface");
        return DIENUM_STOP; // Found and created successfully
    }

    return DIENUM_CONTINUE;
}

// Device lists for better error messages
static BOOL CALLBACK ListDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID*)
{
    const std::wstring deviceName = ToWideString(pdidInstance->tszProductName);

    LogMessage(L"[INFO] Available device: " + deviceName);
    return DIENUM_CONTINUE; // Continue enumerating all devices
}

void DirectInput::ListAvailableDevices()
{
    if (directInput)
    {
        LogMessage(L"[INFO] Enumerating available game controllers:");
        const HRESULT hr = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, ListDevicesCallback, NULL, DIEDFL_ATTACHEDONLY);
        if (FAILED(hr))
        {
            LogMessage(L"[ERROR] Failed to enumerate devices: 0x" + std::to_wstring(hr));
        }
        else
        {
            LogMessage(L"[INFO] Device enumeration complete");
        }
    }
    else
    {
        LogMessage(L"[ERROR] DirectInput not initialized, cannot list devices");
    }
}

void DirectInput::ShowAvailableDevicesOnConsole()
{
    if (directInput)
    {
        directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, ConsoleListDevicesCallback, NULL, DIEDFL_ATTACHEDONLY);
    }
    else
    {
        std::wcout << L"  (Could not enumerate devices - DirectInput not initialized)" << L'\n';
    }
}

// Kick-off DirectInput
bool DirectInput::InitializeDevice(FFBDevice& device)
{
    LogMessage(L"[INFO] Initializing DirectInput...");

    HRESULT hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<VOID**>(&directInput), NULL);
    if (FAILED(hr))
    {
        LogMessage(L"[ERROR] DirectInput8Create failed: 0x" + std::to_wstring(hr));
        return false;
    }

    LogMessage(L"[INFO] Searching for device: " + device.config.targetDeviceName);
    EnumDeviceHelper edh(&device, device.config.targetDeviceName);
    hr = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, &edh, DIEDFL_ATTACHEDONLY);

    if (FAILED(hr))
    {
        LogMessage(L"[ERROR] EnumDevices failed: 0x" + std::to_wstring(hr));
        return false;
    }

    if (device.diDevice == NULL)
    {
        LogMessage(L"[ERROR] Device not found: " + device.config.targetDeviceName);
        return false;
    }

    LogMessage(L"[INFO] Device initialization successful");
    return true;
}
