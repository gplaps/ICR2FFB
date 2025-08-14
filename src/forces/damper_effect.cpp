#include "damper_effect.h"
#include "constants.h"
#include "helpers.h"
#include <dinput.h>
#include <iostream>
#include <algorithm>


// Create damper to make it feel like the steering is not powered, mostly for pitlane, maybe hairpin use
// Only goes to '40mph'

void UpdateDamperEffect(double speedMph, IDirectInputEffect* effect, double masterForceScale, double damperForceScale) {
    if (!effect) return;

    double maxSpeed = 40.0;
    double minDamper = 0.0; (void)minDamper; // unused currently
    double maxDamper = 5000.0;

    double t = std::clamp(speedMph / maxSpeed, 0.0, 1.0);
    LONG damperStrength = static_cast<LONG>(((1.0 - t) * maxDamper * masterForceScale) * damperForceScale);

    DICONDITION condition = {};
    condition.lOffset = 0;
    condition.lPositiveCoefficient = damperStrength;
    condition.lNegativeCoefficient = damperStrength;
    condition.dwPositiveSaturation = DEFAULT_DINPUT_GAIN;
    condition.dwNegativeSaturation = DEFAULT_DINPUT_GAIN;
    condition.lDeadBand = 0;

    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.cAxes = 1;
    DWORD axes[1] = { DIJOFS_X };
    LONG dir[1] = { 0 };
    eff.rgdwAxes = axes;
    eff.rglDirection = dir;
    eff.cbTypeSpecificParams = sizeof(DICONDITION);
    eff.lpvTypeSpecificParams = &condition;

    HRESULT hr = effect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);
    if (FAILED(hr)) {
        std::wcerr << L"Failed to update damper effect: 0x" << std::hex << hr << std::endl;
    }
    if (!effect) return;
}
