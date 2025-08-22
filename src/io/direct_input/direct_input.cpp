#include "direct_input.h"

#include "ffb_device.h"
#include "log.h"
#include "string_utilities.h"

#include <iostream>
#include <string>

// "singleton"
DirectInput* directInput = NULL;

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

struct EnumDeviceHelper2
{
    explicit EnumDeviceHelper2(DirectInput* d) :
        di(d) {}
    DirectInput* di;
};

DirectInput::DirectInputDevice* DirectInput::GetDevice(const std::wstring& productName)
{
    for (size_t i = 0; i < knownDevices.size(); ++i)
    {
        if (productName == ToWideString(knownDevices[i].instance.tszProductName))
        {
            return &knownDevices[i];
        }
    }
    LogMessage(L"[ERROR] Failed to find device by name: " + productName);
    return NULL;
}

DirectInput::DirectInputDevice* DirectInput::GetDevice(size_t index)
{
    for (size_t i = 0; i < knownDevices.size(); ++i)
    {
        if (i == index - 1)
        {
            return &knownDevices[i];
        }
    }
    LogMessage(L"[ERROR] Failed to find device by index: " + std::to_wstring(index) + L" is out of range " + std::to_wstring(knownDevices.size() + 1));
    return NULL;
}

// consider changing the following three to IDirectInputDevice8 ... thats where this rework is undecided and becomes unclean
FFBDevice* DirectInput::CreateDevice(const std::wstring& productName)
{
    DirectInputDevice* device = GetDevice(productName);
    return CreateDeviceInternal(device);
}

FFBDevice* DirectInput::CreateDevice(size_t index)
{
    DirectInputDevice* device = GetDevice(index);
    return CreateDeviceInternal(device);
}

FFBDevice* DirectInput::CreateDeviceInternal(DirectInputDevice* device)
{
    if (device)
    {
        const HRESULT hr = DirectInput::directInputObject->CreateDevice(device->instance.guidInstance, &device->runtime, NULL);
        if (FAILED(hr))
        {
            LogMessage(L"[ERROR] Failed to create device: 0x" + std::to_wstring(hr));
            return NULL;
        }
        else
        {
            return new FFBDevice(device->runtime, ToWideString(device->instance.tszProductName));
        }
    }
    return NULL;
}

static FFBDevice* CreateTheDeviceFromConfig(const FFBConfig& config)
{
    FFBDevice*          device     = NULL;
    const std::wstring& deviceName = config.GetString(L"base", L"device");
    // by index
    try
    {
        unsigned long targetIndex = std::stoul(deviceName);
        if (targetIndex > 0)
        {
            device = directInput->CreateDevice(targetIndex);
            if (device)
            {
                LogMessage(L"[INFO] Found matching device by index " + std::to_wstring(targetIndex) + L": " + deviceName);
            }
        }
    }
    catch (const std::exception&) // NOLINT(bugprone-empty-catch)
    {
        // not a number
    }

    // by name
    if (!device)
    {
        device = directInput->CreateDevice(deviceName);
    }

    if (!device)
    {
        LogMessage(L"[ERROR] Failed to find device " + deviceName);
    }
    return device;
}

// Fill Device lists
static BOOL CALLBACK ListDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
    DirectInput* edh2 = static_cast<DirectInput*>(pContext);
    edh2->knownDevices.push_back(DirectInput::DirectInputDevice(*pdidInstance));
    return DIENUM_CONTINUE; // Continue enumerating all devices
}

void DirectInput::UpdateAvailableDevice()
{
    if (directInputObject)
    {
        LogMessage(L"[INFO] Enumerating available game controllers");
        knownDevices.clear();
        EnumDeviceHelper2 edh2(this);
        const HRESULT     hr = directInputObject->EnumDevices(DI8DEVCLASS_GAMECTRL, ListDevicesCallback, &edh2, DIEDFL_ATTACHEDONLY);
        if (FAILED(hr))
        {
            LogMessage(L"[ERROR] Failed to enumerate devices: 0x" + std::to_wstring(hr));
        }
    }
    else
    {
        LogMessage(L"[ERROR] DirectInput not initialized, cannot list devices");
    }
}

std::vector<std::wstring> DirectInput::AvailableDevices() const
{
    std::vector<std::wstring> deviceNames;
    deviceNames.reserve(knownDevices.size());
    for (size_t i = 0; i < knownDevices.size(); ++i)
    {
        deviceNames.push_back(ToWideString(knownDevices[i].instance.tszProductName));
    }
    return deviceNames;
}

// Kick-off DirectInput
bool DirectInput::InitializeDevice(FFBDevice& device)
{
    // LogMessage(L"[INFO] Searching for device: " + device.config.GetString(L"base", L"device"));
    // EnumDeviceHelper edh(&device, device.config.GetString(L"base", L"device"));
    // HRESULT          hr = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumDevicesCallback, &edh, DIEDFL_ATTACHEDONLY);

    // if (FAILED(hr))
    // {
    //     LogMessage(L"[ERROR] EnumDevices failed: 0x" + std::to_wstring(hr));
    //     return false;
    // }

    // if (device.diDevice == NULL)
    // {
    //     LogMessage(L"[ERROR] Device not found: " + device.config.GetString(L"base", L"device"));
    //     return false;
    // }

    // LogMessage(L"[INFO] Device initialization successful");
    return true;
}

DirectInput::DirectInput() :
    knownDevices(),
    directInputObject(NULL),
    mInitialized(false)
{
    if (!Initialize())
    {
        return;
    }
    UpdateAvailableDevice();

    mInitialized = true;
}

bool DirectInput::Initialize()
{
    LogMessage(L"[INFO] Initializing DirectInput...");
    HRESULT hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<VOID**>(&directInputObject), NULL);
    if (FAILED(hr) || directInputObject == NULL)
    {
        LogMessage(L"[ERROR] DirectInput8Create failed: 0x" + std::to_wstring(hr));
        return false;
    }
    return true;
}

bool DirectInput::Valid() const
{
    return mInitialized;
}

void DirectInput::LogDeviceList() const
{
    // Show available devices on console
    const std::vector<std::wstring>& deviceList = AvailableDevices();
    for (size_t i = 0; i < deviceList.size(); ++i)
    {
        // user facing is indices starting at 1
        std::wcout << std::to_wstring(i + 1) << deviceList[i] << L'\n';
    }
}
