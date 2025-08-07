#include "constant_force.h"
#include <iostream>
#include <algorithm>
#include <deque>
#include <numeric>

void ApplyConstantForceEffect(const CalculatedLateralLoad& load, IDirectInputEffect* constantForceEffect) {
    if (!constantForceEffect) return;

    constexpr double maxG = 8.0;
    constexpr double exponent = 0.85;
    constexpr int minChange = 100;
    constexpr double dlongThreshold = 0.01;  // Minimum change to consider movement
    constexpr int freezeFramesThreshold = 10;



    // Step 1: Scale lateral G with curve
    double signedG = std::clamp(load.lateralG, -maxG, maxG);
    double curved = std::pow(std::abs(signedG) / maxG, exponent);

    // Step 2: Smoothing (moving average)
    static std::deque<double> forceHistory;
    forceHistory.push_back(curved);
    if (forceHistory.size() > 5) {
        forceHistory.pop_front();
    }
    double smoothed = std::accumulate(forceHistory.begin(), forceHistory.end(), 0.0) / forceHistory.size();
    int magnitude = static_cast<int>(smoothed * 10000.0);

    // Step 3: Smooth direction changes
    static LONG lastDirection = 0;
    constexpr double directionSmoothingFactor = 0.2;
    LONG targetDir = load.directionVal;

    // Blend direction near center to avoid sharp swaps
    lastDirection = static_cast<LONG>((1.0 - directionSmoothingFactor) * lastDirection + directionSmoothingFactor * targetDir);

    // Step 4: Avoid micro-changes
    static int lastMagnitude = -1;
    if (std::abs(magnitude - lastMagnitude) < minChange && std::abs(lastDirection - targetDir) < 500) return;
    lastMagnitude = magnitude;

    // Step 5: Apply effect
    DICONSTANTFORCE cf = { magnitude };
    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwGain = 10000;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.cAxes = 1;
    DWORD axes[1] = { DIJOFS_X };
    LONG dir[1] = { lastDirection };
    eff.rgdwAxes = axes;
    eff.rglDirection = dir;
    eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;

    HRESULT hr = constantForceEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS | DIEP_DIRECTION);
    if (FAILED(hr)) {
        std::wcerr << L"Constant force SetParameters failed: 0x" << std::hex << hr << std::endl;
    }
}