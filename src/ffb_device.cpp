#include "ffb_device.h"

#include "constants.h"
#include "direct_input.h"
#include "helpers.h"
#include "log.h"

#include <iostream>

FFBDevice::FFBDevice(const FFBConfig& configIn) :
    config(configIn),
    js() {}

// === Force Effect Creators ===
void FFBDevice::CreateConstantForceEffect()
{
    if (!diDevice) return;

    DICONSTANTFORCE cf        = {0};

    DIEFFECT eff              = {};
    eff.dwSize                = sizeof(DIEFFECT);
    eff.dwFlags               = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration            = INFINITE;
    eff.dwGain                = DEFAULT_DINPUT_GAIN;
    eff.dwTriggerButton       = DIEB_NOTRIGGER;
    eff.cAxes                 = 1;
    DWORD axes[1]             = {DIJOFS_X};
    LONG  dir[1]              = {0};
    eff.rgdwAxes              = axes;
    eff.rglDirection          = dir;
    eff.cbTypeSpecificParams  = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;

    DIPROPRANGE diprg         = {};
    diprg.diph.dwSize         = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize   = sizeof(DIPROPHEADER);
    diprg.diph.dwHow          = DIPH_BYOFFSET;
    diprg.diph.dwObj          = DIJOFS_X;
    diprg.lMin                = -static_cast<LONG>(DEFAULT_DINPUT_GAIN);
    diprg.lMax                = DEFAULT_DINPUT_GAIN;
    diDevice->SetProperty(DIPROP_RANGE, &diprg.diph);

    const HRESULT hr = diDevice->CreateEffect(GUID_ConstantForce, &eff, &constantForceEffect, NULL);
    if (FAILED(hr))
        LogMessage(L"[ERROR] Failed to create constant force effect. HRESULT: 0x" + std::to_wstring(hr));
    else
        LogMessage(L"[INFO] Initial constant force created");
}

void FFBDevice::CreateDamperEffect()
{
    if (!diDevice) return;

    DICONDITION condition          = {};
    condition.lOffset              = 0;
    condition.lPositiveCoefficient = 8000;
    condition.lNegativeCoefficient = 8000;
    condition.dwPositiveSaturation = DEFAULT_DINPUT_GAIN;
    condition.dwNegativeSaturation = DEFAULT_DINPUT_GAIN;
    condition.lDeadBand            = 0;

    DIEFFECT eff                   = {};
    eff.dwSize                     = sizeof(DIEFFECT);
    eff.dwFlags                    = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration                 = INFINITE;
    eff.dwGain                     = DEFAULT_DINPUT_GAIN;
    eff.dwTriggerButton            = DIEB_NOTRIGGER;
    eff.cAxes                      = 1;
    DWORD axes[1]                  = {DIJOFS_X};
    LONG  dir[1]                   = {0};
    eff.rgdwAxes                   = axes;
    eff.rglDirection               = dir;
    eff.cbTypeSpecificParams       = sizeof(DICONDITION);
    eff.lpvTypeSpecificParams      = &condition;

    const HRESULT hr               = diDevice->CreateEffect(GUID_Damper, &eff, &damperEffect, NULL);
    if (FAILED(hr) || !damperEffect)
        LogMessage(L"[ERROR] Failed to create damper effect. HRESULT: 0x" + std::to_wstring(hr));
    else
        LogMessage(L"[INFO] Initial damper effect created");
}

void FFBDevice::CreateSpringEffect()
{
    if (!diDevice) return;

    DICONDITION condition          = {};
    condition.lOffset              = 0;
    condition.lPositiveCoefficient = 8000;
    condition.lNegativeCoefficient = 8000;
    condition.dwPositiveSaturation = DEFAULT_DINPUT_GAIN;
    condition.dwNegativeSaturation = DEFAULT_DINPUT_GAIN;
    condition.lDeadBand            = 0;

    DIEFFECT eff                   = {};
    eff.dwSize                     = sizeof(DIEFFECT);
    eff.dwFlags                    = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration                 = INFINITE;
    eff.dwGain                     = DEFAULT_DINPUT_GAIN;
    eff.dwTriggerButton            = DIEB_NOTRIGGER;
    eff.cAxes                      = 1;
    DWORD axes[1]                  = {DIJOFS_X};
    LONG  dir[1]                   = {0};
    eff.rgdwAxes                   = axes;
    eff.rglDirection               = dir;
    eff.cbTypeSpecificParams       = sizeof(DICONDITION);
    eff.lpvTypeSpecificParams      = &condition;

    const HRESULT hr               = diDevice->CreateEffect(GUID_Spring, &eff, &springEffect, NULL);
    if (FAILED(hr) || !springEffect)
        LogMessage(L"[ERROR] Failed to create spring effect. HRESULT: 0x" + std::to_wstring(hr));
    else
        LogMessage(L"[INFO] Initial spring effect created");
}

void FFBDevice::UpdateDamperEffect(LONG damperStrength)
{
    if (!damperEffect) return;

    DICONDITION condition          = {};
    condition.lOffset              = 0;
    condition.lPositiveCoefficient = damperStrength;
    condition.lNegativeCoefficient = damperStrength;
    condition.dwPositiveSaturation = DEFAULT_DINPUT_GAIN;
    condition.dwNegativeSaturation = DEFAULT_DINPUT_GAIN;
    condition.lDeadBand            = 0;

    DIEFFECT eff                   = {};
    eff.dwSize                     = sizeof(DIEFFECT);
    eff.dwFlags                    = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.cAxes                      = 1;
    DWORD axes[1]                  = {DIJOFS_X};
    LONG  dir[1]                   = {0};
    eff.rgdwAxes                   = axes;
    eff.rglDirection               = dir;
    eff.cbTypeSpecificParams       = sizeof(DICONDITION);
    eff.lpvTypeSpecificParams      = &condition;

    const HRESULT hr               = damperEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
    if (FAILED(hr))
        std::wcerr << L"Failed to update damper effect: 0x" << std::hex << hr << std::endl;
}

void FFBDevice::UpdateSpringEffect(LONG springStrength)
{
    if (!springEffect) return;

    DICONDITION condition          = {};
    condition.lOffset              = 0;
    condition.lPositiveCoefficient = springStrength;
    condition.lNegativeCoefficient = springStrength;
    condition.dwPositiveSaturation = DEFAULT_DINPUT_GAIN;
    condition.dwNegativeSaturation = DEFAULT_DINPUT_GAIN;
    condition.lDeadBand            = 0; // Reduce deadzone

    DWORD axes[1]                  = {DIJOFS_X};
    LONG  direction[1]             = {0};

    DIEFFECT eff                   = {};
    eff.dwSize                     = sizeof(DIEFFECT);
    eff.dwFlags                    = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration                 = INFINITE;
    eff.dwSamplePeriod             = 0;
    eff.dwGain                     = DI_FFNOMINALMAX;
    eff.cAxes                      = 1;
    eff.rgdwAxes                   = axes;
    eff.rglDirection               = direction;
    eff.lpEnvelope                 = NULL;
    eff.cbTypeSpecificParams       = sizeof(DICONDITION);
    eff.lpvTypeSpecificParams      = &condition;
    eff.dwStartDelay               = 0;

    const HRESULT hr               = springEffect->SetParameters(&eff, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS);
    if (FAILED(hr))
        std::wcerr << L"[ERROR] Failed to update spring effect: 0x" << std::hex << hr << std::endl;
}

void FFBDevice::UpdateConstantForceEffect(LONG magnitude, bool withDirection)
{
    if (!constantForceEffect) return;

    DICONSTANTFORCE cf        = {magnitude};
    DIEFFECT        eff       = {};
    eff.dwSize                = sizeof(DIEFFECT);
    eff.dwFlags               = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration            = INFINITE;
    eff.dwGain                = DEFAULT_DINPUT_GAIN;
    eff.dwTriggerButton       = DIEB_NOTRIGGER;
    eff.cAxes                 = 1;
    DWORD axes[1]             = {DIJOFS_X};
    LONG  dir[1]              = {0}; // ← Always zero direction now, this broke Moza wheels
    eff.rgdwAxes              = axes;
    eff.rglDirection          = dir;
    eff.cbTypeSpecificParams  = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;

    // Only set magnitude params, skip direction
    const DWORD   parameters = static_cast<DWORD>(withDirection ? DIEP_TYPESPECIFICPARAMS | DIEP_DIRECTION : DIEP_TYPESPECIFICPARAMS);
    const HRESULT hr         = constantForceEffect->SetParameters(&eff, parameters);
    if (FAILED(hr))
        std::wcerr << L"Constant force SetParameters failed: 0x" << std::hex << hr << std::endl;
}

void FFBDevice::StartConstant()
{
    if (!constantStarted)
    {
        constantForceEffect->Start(1, 0);
        constantStarted = true;
        LogMessage(L"[INFO] Constant force started");
    }
}

void FFBDevice::StartDamper()
{
    if (!damperStarted && damperEffect)
    {
        damperEffect->Start(1, 0);
        damperStarted = true;
        LogMessage(L"[INFO] Damper effect started");
    }
}

void FFBDevice::StartSpring()
{
    if (!springStarted && springEffect)
    {
        springEffect->Start(1, 0);
        springStarted = true;
        LogMessage(L"[INFO] Spring effect started");
    }
}

int FFBDevice::InitDevice()
{
    // Initialize DirectInput device
    // I don't really know how this works yet. It enabled "exclusive" mode on the device and calls it a "type 2" joystick.. OK!
    if (!DirectInput::InitializeDevice(*this))
    {
        LogMessage(L"[ERROR] Failed to initialize DirectInput or find device: " + config.targetDeviceName);
        LogMessage(L"[ERROR] Available devices:");

        // List available devices to help user
        DirectInput::ListAvailableDevices();

        LogMessage(L"[ERROR] Check your ffb.ini file - device name must match exactly");

        // SHOW ERROR ON CONSOLE immediately
        std::wcout << L"[ERROR] Could not find controller: " << config.targetDeviceName << std::endl;
        std::wcout << L"[ERROR] Available devices:" << std::endl;

        // Show available devices on console too
        DirectInput::ShowAvailableDevicesOnConsole();

        std::wcout << L"[ERROR] Check your ffb.ini file - device name must match exactly" << std::endl;
        std::wcout << L"Press any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    LogMessage(L"[INFO] Device found and initialized successfully");

    return DirectInputSetup();
}

int FFBDevice::DirectInputSetup() const
{
    // ONLY configure device if it was found successfully
    HRESULT hr;
    hr = diDevice->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr))
    {
        LogMessage(L"[ERROR] Failed to set data format: 0x" + std::to_wstring(hr));

        std::wcout << L"[ERROR] Failed to set data format: 0x" << std::hex << hr << std::endl;
        std::wcout << L"Press any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    hr = diDevice->SetCooperativeLevel(GetConsoleWindow(), DISCL_BACKGROUND | DISCL_EXCLUSIVE);
    if (FAILED(hr))
    {
        LogMessage(L"[ERROR] Failed to set cooperative level: 0x" + std::to_wstring(hr));
        LogMessage(L"[ERROR] Another application may be using the device exclusively");

        std::wcout << L"[ERROR] Failed to set cooperative level: 0x" << std::hex << hr << std::endl;
        std::wcout << L"[ERROR] Another application may be using the device exclusively" << std::endl;
        std::wcout << L"Press any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    hr = diDevice->Acquire();
    if (FAILED(hr))
        LogMessage(L"[WARNING] Initial acquire failed: 0x" + std::to_wstring(hr) + L" (this is often normal)");
    else
        LogMessage(L"[INFO] Device acquired successfully");

    return 0;
}

void FFBDevice::Start()
{
}

void FFBDevice::Update()
{
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
