#include "ffb_effect.h"

#include "constants.h"
#include "log.h"
#include "string_utilities.h" // IWYU pragma: keep

#include <iostream>

FFBEffect::~FFBEffect() {}

void FFBEffect::Start()
{
    if (!started)
    {
        effect->Start(1, 0);
        started = true;
        LogMessage(L"[INFO] " + effectName + L" started");
    }
}

DiConstantEffect::DiConstantEffect(IDirectInputDevice8* device) :
    FFBEffect(L"Constant force")
{
    if (!device) { return; }

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
    device->SetProperty(DIPROP_RANGE, &diprg.diph);

    const HRESULT hr = device->CreateEffect(GUID_ConstantForce, &eff, &effect, NULL);
    if (FAILED(hr))
    {
        LogMessage(L"[ERROR] Failed to create constant force effect. HRESULT: 0x" + std::to_wstring(hr));
    }
    else
    {
        LogMessage(L"[INFO] Initial constant force created");
    }
}

void DiConstantEffect::Update(LONG magnitude, bool withDirection)
{
    if (!effect) { return; }

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
    const HRESULT hr         = effect->SetParameters(&eff, parameters);
    if (FAILED(hr))
    {
        std::wcerr << L"Constant force SetParameters failed: 0x" << std::hex << hr << L'\n';
    }
}

DiDamperEffect::DiDamperEffect(IDirectInputDevice8* device) :
    FFBEffect(L"Damper")
{
    if (!device) { return; }

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

    const HRESULT hr               = device->CreateEffect(GUID_Damper, &eff, &effect, NULL);
    if (FAILED(hr) || !effect)
    {
        LogMessage(L"[ERROR] Failed to create damper effect. HRESULT: 0x" + std::to_wstring(hr));
    }
    else
    {
        LogMessage(L"[INFO] Initial damper effect created");
    }
}

void DiDamperEffect::Update(LONG magnitude, bool /*withDirection*/)
{
    if (!effect) { return; }

    DICONDITION condition          = {};
    condition.lOffset              = 0;
    condition.lPositiveCoefficient = magnitude;
    condition.lNegativeCoefficient = magnitude;
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

    const HRESULT hr               = effect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
    if (FAILED(hr))
    {
        std::wcerr << L"Failed to update damper effect: 0x" << std::hex << hr << L'\n';
    }
}

DiSpringEffect::DiSpringEffect(IDirectInputDevice8* device) :
    FFBEffect(L"Spring")
{
    if (!device) { return; }

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

    const HRESULT hr               = device->CreateEffect(GUID_Spring, &eff, &effect, NULL);
    if (FAILED(hr) || !effect)
    {
        LogMessage(L"[ERROR] Failed to create spring effect. HRESULT: 0x" + std::to_wstring(hr));
    }
    else
    {
        LogMessage(L"[INFO] Initial spring effect created");
    }
}

void DiSpringEffect::Update(LONG magnitude, bool /*withDirection*/)
{
    if (!effect) { return; }

    DICONDITION condition          = {};
    condition.lOffset              = 0;
    condition.lPositiveCoefficient = magnitude;
    condition.lNegativeCoefficient = magnitude;
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

    const HRESULT hr               = effect->SetParameters(&eff, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS);
    if (FAILED(hr))
    {
        std::wcerr << L"[ERROR] Failed to update spring effect: 0x" << std::hex << hr << L'\n';
    }
}
