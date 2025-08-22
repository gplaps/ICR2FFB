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
    spring(NULL)
{
    if (!InitDevice(name))
    {
        return;
    }
    InitEffects(config);
    mInitialized = diDevice && (constant || damper || spring);
}

FFBDevice::~FFBDevice() {}

void FFBDevice::InitEffects(const FFBConfig& config)
{
    if (mInitialized) { return; }

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

bool FFBDevice::InitDevice(const std::wstring& productNameOrIndex)
{
    diDevice = DirectInput::Instance()->InitializeDevice(productNameOrIndex);
    if (!diDevice)
    {
        LogMessage(L"[ERROR] Failed to initialize DirectInput device: " + productNameOrIndex);
        LogMessage(L"[ERROR] Check your ffb.ini file - device name must match exactly");
        // // SHOW ERROR ON CONSOLE immediately
        std::wcout << L"[ERROR] Could not find controller: " << productNameOrIndex << L'\n';
        std::wcout << L"[ERROR] Check your ffb.ini file - device name must match exactly" << L'\n';
        // std::wcout << L"Press any key to exit..." << L'\n';
        // std::cin.get();
        return false;
    }

    LogMessage(L"[INFO] Device found and initialized successfully");

    return DirectInputSetup();
}

// matter of taste if this should be moved into InitDevice
// I don't really know how this works yet. It enabled "exclusive" mode on the device and calls it a "type 2" joystick.. OK!
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
