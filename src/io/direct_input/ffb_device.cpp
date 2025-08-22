#include "ffb_device.h"

#include "constants.h"
#include "direct_input.h"
#include "ffb_effect.h"
#include "log.h"
#include "string_utilities.h" // IWYU pragma: keep

#include <iostream>

FFBDevice::FFBDevice(const FFBConfig& config, const std::wstring& name) :
    diDevice(NULL),
    js(),
    constant(NULL),
    damper(NULL),
    spring(NULL),
    productName(name)
{
    // InitDevice();
    InitEffects(config);
    if (!DirectInputSetup())
    {
        return;
    }
    mInitialized = true;
}

// FFBDevice::FFBDevice(IDirectInputDevice8* diPtr, const std::wstring& name) :
//     diDevice(diPtr),
//     js(),
//     constant(NULL),
//     damper(NULL),
//     spring(NULL),
//     productName(name)
// {
//     if (!DirectInputSetup())
//     {
//         return;
//     }
//     mInitialized = true;
// }

void FFBDevice::InitEffects(const FFBConfig& config)
{
    if (!mInitialized) { return; }

    // Create FFB effects as needed
    if (config.GetBool(L"effects", L"constant"))
    {
        constant = new DiConstantEffect(diDevice);
    }
    if (config.GetBool(L"effects", L"damper"))
    {
        damper = new DiDamperEffect(diDevice);
    }
    if (config.GetBool(L"effects", L"spring"))
    {
        spring = new DiSpringEffect(diDevice);
    }
}

void FFBDevice::Update(double constantStrength, double damperStrength, double springStrength, bool constantWithDirection)
{
    if (constant)
    {
        double constantDirectInput = constantStrength * DEFAULT_DINPUT_GAIN_DBL;
        constant->Update(static_cast<LONG>(constantDirectInput), constantWithDirection);
    }
    if (damper)
    {
        double damperDirectInput = damperStrength * DEFAULT_DINPUT_GAIN_DBL;
        damper->Update(static_cast<LONG>(damperDirectInput));
    }
    if (spring)
    {
        double springDirectInput = springStrength * DEFAULT_DINPUT_GAIN_DBL;
        spring->Update(static_cast<LONG>(springDirectInput));
    }
}

void FFBDevice::Start()
{
    if (constant) { constant->Start(); }
    if (damper) { damper->Start(); }
    if (spring) { spring->Start(); }
}

// int FFBDevice::InitDevice()
// {
//     // Initialize DirectInput device
//     // I don't really know how this works yet. It enabled "exclusive" mode on the device and calls it a "type 2" joystick.. OK!
//     if (!directInput->InitializeDevice(*this))
//     {
//         // LogMessage(L"[ERROR] Failed to initialize DirectInput or find device: " + config.GetString(L"base", L"device"));
//         // LogMessage(L"[ERROR] Available devices:");

//         // // List available devices to help user
//         // directInput->UpdateAvailableDevice();

//         // LogMessage(L"[ERROR] Check your ffb.ini file - device name must match exactly");

//         // // SHOW ERROR ON CONSOLE immediately
//         // std::wcout << L"[ERROR] Could not find controller: " << config.GetString(L"base", L"device") << L'\n';
//         // std::wcout << L"[ERROR] Available devices:" << L'\n';

//         DirectInput::LogDeviceList();
//         std::wcout << L"[ERROR] Check your ffb.ini file - device name must match exactly" << L'\n';
//         std::wcout << L"Press any key to exit..." << L'\n';
//         std::cin.get();
//         return 1;
//     }

//     LogMessage(L"[INFO] Device found and initialized successfully");

//     return DirectInputSetup();
// }

bool FFBDevice::DirectInputSetup() const
{
    // ONLY configure device if it was found successfully
    HRESULT hr = diDevice->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr))
    {
        LogMessage(L"[ERROR] Failed to set data format: 0x" + std::to_wstring(hr));
        return false;
    }

#if defined(_WIN32_WINNT) && _WIN32_WINNT >= _WIN32_WINNT_WIN2K
    hr = diDevice->SetCooperativeLevel(GetConsoleWindow(), DISCL_BACKGROUND | DISCL_EXCLUSIVE);
    if (FAILED(hr))
    {
        LogMessage(L"[ERROR] Failed to set cooperative level: 0x" + std::to_wstring(hr));
        LogMessage(L"[ERROR] Another application may be using the device exclusively");
        return false;
    }
#endif

    hr = diDevice->Acquire();
    if (FAILED(hr))
    {
        LogMessage(L"[WARNING] Initial acquire failed: 0x" + std::to_wstring(hr) + L" (this is often normal)");
    }
    else
    {
        LogMessage(L"[INFO] Device acquired successfully");
    }

    return true;
}

void FFBDevice::Poll()
{
    // Poll input state
    if (FAILED(diDevice->Poll()))
    {
        diDevice->Acquire();
        diDevice->Poll();
    }
    diDevice->GetDeviceState(sizeof(DIJOYSTATE2), &js);
}
