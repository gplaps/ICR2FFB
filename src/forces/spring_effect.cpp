#include "spring_effect.h"
#include "constants.h"
#include "project_dependencies.h"
#include <iostream>
#include <algorithm>

// just basic centering spring to try to give the wheel more weight while driving
// Used to scale to speed but ive never found this effect to feel very nice on the fanatec

void UpdateSpringEffectImpl(IDirectInputEffect* effect, double springStrength) {
    if (!effect) return;

    DICONDITION condition = {};
    condition.lOffset = 0;
    condition.lPositiveCoefficient = static_cast<LONG>(springStrength);
    condition.lNegativeCoefficient = static_cast<LONG>(springStrength);
    condition.dwPositiveSaturation = DEFAULT_DINPUT_GAIN;
    condition.dwNegativeSaturation = DEFAULT_DINPUT_GAIN;
    condition.lDeadBand = 0; // Reduce deadzone

    DWORD axes[1] = { DIJOFS_X };
    LONG direction[1] = { 0 };

    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwSamplePeriod = 0;
    eff.dwGain = DI_FFNOMINALMAX;
    eff.cAxes = 1;
    eff.rgdwAxes = axes;
    eff.rglDirection = direction;
    eff.lpEnvelope = NULL;
    eff.cbTypeSpecificParams = sizeof(DICONDITION);
    eff.lpvTypeSpecificParams = &condition;
    eff.dwStartDelay = 0;

    HRESULT hr = effect->SetParameters(&eff, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS);
    if (FAILED(hr)) {
        std::wcerr << L"[ERROR] Failed to update spring effect: 0x" << std::hex << hr << std::endl;
    }
}

void UpdateSpringEffect(IDirectInputEffect* effect, double masterForceScale) {
    // How much centering force?
    LONG springStrength = static_cast<LONG>(6500.0 * masterForceScale);

    UpdateSpringEffectImpl(effect, springStrength);
}
