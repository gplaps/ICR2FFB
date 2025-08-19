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
        targetDeviceName(name),
        currentIndex(1) {}
    FFBDevice*   device;
    std::wstring targetDeviceName;
    int          currentIndex;
};

static BOOL CALLBACK ConsoleListDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
    int*               index      = static_cast<int*>(pContext);
    const std::wstring deviceName = ToWideString(pdidInstance->tszProductName);
    std::wcout << L"  " << *index << L": " << deviceName << L'\n';
    (*index)++;
    return DIENUM_CONTINUE;
}

static BOOL CALLBACK EnumDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
    if (!pContext) { return DIENUM_CONTINUE; }

    EnumDeviceHelper* edh         = static_cast<EnumDeviceHelper*>(pContext);
    edh->currentIndex             = 1;
    const std::wstring deviceName = ToWideString(pdidInstance->tszProductName);

    // First try exact name match
    if (deviceName == edh->targetDeviceName)
    {
        LogMessage(L"[INFO] Found matching device by name: " + deviceName);
        const HRESULT hr = DirectInput::directInput->CreateDevice(pdidInstance->guidInstance, &edh->device->diDevice, NULL);
        if (FAILED(hr))
        {
            LogMessage(L"[ERROR] Failed to create device: 0x" + std::to_wstring(hr));
            edh->currentIndex++;
            return DIENUM_CONTINUE;
        }
        LogMessage(L"[INFO] Successfully created device interface");
        edh->currentIndex = 1; // Reset for next time
        return DIENUM_STOP;
    }

    // If name didn't match, check if targetDeviceName is a number (index)
    try
    {
        int targetIndex = std::stoi(edh->targetDeviceName);
        if (edh->currentIndex == targetIndex)
        {
            LogMessage(L"[INFO] Found matching device by index " + std::to_wstring(edh->currentIndex) + L": " + deviceName);
            const HRESULT hr = DirectInput::directInput->CreateDevice(pdidInstance->guidInstance, &edh->device->diDevice, NULL);
            if (FAILED(hr))
            {
                LogMessage(L"[ERROR] Failed to create device: 0x" + std::to_wstring(hr));
                edh->currentIndex++;
                return DIENUM_CONTINUE;
            }
            LogMessage(L"[INFO] Successfully created device interface");
            return DIENUM_STOP;
        }
    }
    catch (const std::exception&) // NOLINT(bugprone-empty-catch)
    {
        // targetDeviceName is not a valid number, that's fine
        // We already tried name matching above
    }

    edh->currentIndex++;
    return DIENUM_CONTINUE;
}

// Device lists for better error messages
static BOOL CALLBACK ListDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
    int*               index      = static_cast<int*>(pContext);
    const std::wstring deviceName = ToWideString(pdidInstance->tszProductName);
    LogMessage(L"[INFO] Available device " + std::to_wstring(*index) + L": " + deviceName);
    (*index)++;
    return DIENUM_CONTINUE; // Continue enumerating all devices
}

void DirectInput::ListAvailableDevices()
{
    if (directInput)
    {
        int index = 1;
        LogMessage(L"[INFO] Enumerating available game controllers:");
        const HRESULT hr = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, ListDevicesCallback, &index, DIEDFL_ATTACHEDONLY);
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
        int index = 1;
        std::wcout << L"Available devices:" << L'\n';
        directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, ConsoleListDevicesCallback, &index, DIEDFL_ATTACHEDONLY);
        std::wcout << L"You can use either the device name or index number in your INI file." << L'\n';
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

    LogMessage(L"[INFO] Searching for device: " + device.config.GetString(L"device"));
    EnumDeviceHelper edh(&device, device.config.GetString(L"device"));
    hr = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, &edh, DIEDFL_ATTACHEDONLY);

    if (FAILED(hr))
    {
        LogMessage(L"[ERROR] EnumDevices failed: 0x" + std::to_wstring(hr));
        return false;
    }

    if (device.diDevice == NULL)
    {
        LogMessage(L"[ERROR] Device not found: " + device.config.GetString(L"device"));
        return false;
    }

    LogMessage(L"[INFO] Device initialization successful");
    return true;
}
