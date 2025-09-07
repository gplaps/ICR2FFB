#include "ffb_device.h"

#include "constants.h"
#include "direct_input.h"
#include "ffb_effect.h"
#include "log.h"
#include "math_utilities.h"   // IWYU pragma: keep
#include "string_utilities.h" // IWYU pragma: keep

#include <cmath>
#include <iostream>

FFBDevice::FFBDevice(const FFBConfig& config, const std::wstring& nameOrIndex, bool optional) :
    diDevice(NULL),
    js(),
    constant(NULL),
    damper(NULL),
    spring(NULL),
    optionalDevice(optional),
    mInitialized(false)
{
    if (!InitDevice(nameOrIndex))
    {
        return;
    }
    InitEffects(config);
    mInitialized = diDevice && (constant || damper || spring);

    (void)optionalDevice; // unused - maybe needed to stop log messages for them
}

FFBDevice::~FFBDevice() {}

bool FFBDevice::Valid() const
{
    return mInitialized;
}

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

static double CheckOutOfRangeValue(double value, const std::wstring& effectName)
{
    if (value > 1.0 || value < -1.0)
    {
        LogMessage(L"[WARNING] Calculated " + effectName + L" effect out of range [-1:1]: " + std::to_wstring(value));
        value = std::clamp(value, -1.0, 1.0);
    }
    if (!std ::isfinite((value)))
    {
        LogMessage(L"NaN encountered in effect: " + effectName);
        value = 0;
    }
    return value;
}

void FFBDevice::Update(double constantStrength, double damperStrength, double springStrength, bool constantWithDirection)
{
    if (constant)
    {
        constantStrength                 = CheckOutOfRangeValue(constantStrength, L"constant");
        const double constantDirectInput = constantStrength * DEFAULT_DINPUT_GAIN_DBL;
        constant->Update(static_cast<LONG>(constantDirectInput), constantWithDirection);
    }
    if (damper)
    {
        damperStrength                 = CheckOutOfRangeValue(damperStrength, L"damper");
        const double damperDirectInput = damperStrength * DEFAULT_DINPUT_GAIN_DBL;
        damper->Update(static_cast<LONG>(damperDirectInput));
    }
    if (spring)
    {
        springStrength                 = CheckOutOfRangeValue(springStrength, L"spring");
        const double springDirectInput = springStrength * DEFAULT_DINPUT_GAIN_DBL;
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
        if (!productNameOrIndex.empty() && !DirectInput::Instance()->AvailableDevices().empty())
        {
            LogMessage(L"[ERROR] Check your ffb.ini file - device name must match exactly");
            std::wcout << L"[ERROR] Check your ffb.ini file - device name must match exactly" << L'\n';
        }
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
