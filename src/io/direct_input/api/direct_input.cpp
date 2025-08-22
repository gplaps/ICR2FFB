#include "direct_input.h"

#include "log.h"
#include "string_utilities.h"
#include "utilities.h"

#include <iostream>
#include <string>

DirectInput* DirectInput::singleton = NULL;

std::vector<DirectInputDevice>& EnumDeviceHelper::AccessKnownDevices() const
{
    return di->knownDevices;
}

DirectInput* DirectInput::Instance()
{
    if (!singleton)
    {
        singleton = new DirectInput();
    }
    return singleton;
}

bool DirectInput::CloseInstance()
{
    if (singleton)
    {
        SAFE_DELETE(singleton);
        return singleton == NULL;
    }
    return false; // depends on use case if this is still a "success" or failure
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
    LogDeviceList();

    mInitialized = true;
}

DirectInput::~DirectInput() {}

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
        // user facing indices starting at 1
        std::wcout << std::to_wstring(i + 1) << deviceList[i] << L'\n';
    }
}

DirectInputDevice* DirectInput::GetDevice(const std::wstring& productName)
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

DirectInputDevice* DirectInput::GetDevice(size_t index)
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

IDirectInputDevice8* DirectInput::CreateDevice(const std::wstring& productName)
{
    DirectInputDevice* device = GetDevice(productName);
    return CreateDeviceInternal(device);
}

IDirectInputDevice8* DirectInput::CreateDevice(size_t index)
{
    DirectInputDevice* device = GetDevice(index);
    return CreateDeviceInternal(device);
}

IDirectInputDevice8* DirectInput::CreateDeviceInternal(DirectInputDevice* device)
{
    if (device)
    {
        const HRESULT hr = DirectInput::directInputObject->CreateDevice(device->instance.guidInstance, &device->runtime, NULL);
        if (SUCCEEDED(hr))
        {
            return device->runtime;
        }
        else
        {
            LogMessage(L"[ERROR] Failed to create device: 0x" + std::to_wstring(hr));
        }
    }
    return NULL;
}

static BOOL CALLBACK ListDevicesCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
    EnumDeviceHelper* edh = static_cast<EnumDeviceHelper*>(pContext);
    edh->AccessKnownDevices().push_back(DirectInputDevice(*pdidInstance));
    return DIENUM_CONTINUE; // Continue enumerating all devices
}

void DirectInput::UpdateAvailableDevice()
{
    if (directInputObject)
    {
        LogMessage(L"[INFO] Enumerating available game controllers");
        knownDevices.clear();
        const HRESULT hr = directInputObject->EnumDevices(DI8DEVCLASS_GAMECTRL, ListDevicesCallback, this, DIEDFL_ATTACHEDONLY);
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

// Kick-off DirectInput - consider renaming CreateDevice and InitializeDevice functions ... currently CreateDevice are the "internal" functions and "InitializeDevice" is the API, but it would be more expressive if CreateDevice is the outwards facing naming
IDirectInputDevice8* DirectInput::InitializeDevice(const std::wstring& productNameOrIndex)
{
    IDirectInputDevice8* device = NULL;
    // by index
    try
    {
        unsigned long targetIndex = std::stoul(productNameOrIndex);
        if (targetIndex > 0)
        {
            device = CreateDevice(targetIndex);
            if (device)
            {
                LogMessage(L"[INFO] Found matching device by index " + std::to_wstring(targetIndex) + L": " + productNameOrIndex);
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
        device = CreateDevice(productNameOrIndex);
        if (device)
        {
            LogMessage(L"[INFO] Found matching device by name: " + productNameOrIndex);
        }
    }

    if (!device)
    {
        LogMessage(L"[ERROR] Failed to find device " + productNameOrIndex);
    }

    return device;
}
