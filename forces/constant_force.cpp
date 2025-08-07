#include "constant_force.h"
#include <iostream>
#include <algorithm>
#include <deque>
#include <numeric>

// Get config data
extern std::wstring targetInvertFFB;

void ApplyConstantForceEffect(const RawTelemetry& current, const RawTelemetry& previous,
    const CalculatedLateralLoad& load, const CalculatedSlip& slip,
    double speed_mph, IDirectInputEffect* constantForceEffect,
    double masterForceScale) {
    if (!constantForceEffect) return;

    // Bunch of logic to try to stop this affect when the game is 'paused' or not running
    // Also if the car is just stopped, the game throws a bunch of random big numbers
    // trying not to break any wrists (or wheels)

    static bool lastCarStopped = false;
    static int stopCounter = 0;
    static const int stopThresholdFrames = 10;  // ~10 updates of consistent status

    bool isCurrentlyStopped = std::abs(current.dlat - previous.dlat) < 0.0001 &&
        std::abs(current.dlong - previous.dlong) < 0.0001;

    if (isCurrentlyStopped) {
        if (!lastCarStopped) {
            stopCounter++;
            if (stopCounter >= stopThresholdFrames) {
                lastCarStopped = true;
                LogMessage(L"[INFO] Constant force skipped: car is stopped.");
            }
        }
    }
    else {
        if (lastCarStopped) {
            stopCounter++;
            if (stopCounter >= stopThresholdFrames) {
                lastCarStopped = false;
                LogMessage(L"[INFO] Car resumed motion, applying constant force.");
            }
        }
    }

    // Reset the counter if status is stable
    if ((isCurrentlyStopped && lastCarStopped) || (!isCurrentlyStopped && !lastCarStopped)) {
        stopCounter = 0;
    }

    if (lastCarStopped) return;

    // Was first attempt at pausing but just stops forces if car is slow
    // problem is speed stays static when paused or in menus
    if (speed_mph < 5.0) {
        // Optionally send zero force if the effect is still active
        DICONSTANTFORCE cf = { 0 };
        DIEFFECT eff = {};
        eff.dwSize = sizeof(DIEFFECT);
        eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
        eff.dwDuration = INFINITE;
        eff.dwGain = 10000;
        eff.dwTriggerButton = DIEB_NOTRIGGER;
        eff.cAxes = 1;
        DWORD axes[1] = { DIJOFS_X };
        LONG dir[1] = { 0 };
        eff.rgdwAxes = axes;
        eff.rglDirection = dir;
        eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
        eff.lpvTypeSpecificParams = &cf;
        constantForceEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS | DIEP_DIRECTION);
        return;
    }

    // Keep track of previous positions to detect motion
    static double lastDlong = 0.0;
    static double lastDlat = 0.0;
    static int freezeFrameCount = 0;
    constexpr int freezeThreshold = 10;
    constexpr double moveThreshold = 0.01;

    bool positionChanged = std::abs(current.dlong - lastDlong) > moveThreshold || std::abs(current.dlat - lastDlat) > moveThreshold;
    if (positionChanged) {
        freezeFrameCount = 0;
        lastDlong = current.dlong;
        lastDlat = current.dlat;
    }
    else {
        freezeFrameCount++;
    }

    // Disable force if the car hasn't moved recently
    // Might be old can possibly delete
    if (freezeFrameCount >= freezeThreshold) {
        DICONSTANTFORCE cf = { 0 };
        DIEFFECT eff = {};
        eff.dwSize = sizeof(DIEFFECT);
        eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
        eff.dwDuration = INFINITE;
        eff.dwGain = 10000;
        eff.dwTriggerButton = DIEB_NOTRIGGER;
        eff.cAxes = 1;
        DWORD axes[1] = { DIJOFS_X };
        LONG dir[1] = { 0 };
        eff.rgdwAxes = axes;
        eff.rglDirection = dir;
        eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
        eff.lpvTypeSpecificParams = &cf;
        constantForceEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS | DIEP_DIRECTION);
        return;
    }

    constexpr double maxG = 8.0;
    constexpr double exponent = 0.85;
    constexpr int minChange = 100;
    constexpr double dlongThreshold = 0.01;  // Minimum change to consider movement
    constexpr int freezeFramesThreshold = 10;


    // OK Calculation of force, made some kind of 'curve'
    // 100% based on feel and probably can be redone if I can get some data on how other games work

    // Step 1: Linear from 0–0.5G to 1000, then logarithmic to 8000
    double g = std::clamp(load.lateralG, -maxG, maxG);
    double absG = std::abs(g);
    double force = 0.0;

    if (absG <= 0.5) {
        // Linear ramp from 0 to 1000
        force = (absG / 0.5) * 1000.0;
    }
    else {
        //curve of force from .5 g to 8g
        double scale = (absG - 0.5) / (8.0 - 0.5);  // normalized [0, 1]
        double curved = std::log1p(scale * 10.0);   // still [0, ~2.4]

        // new scaling: 0.0 = 1000, 0.5 = 4000, 1.0 = 8000
        double forceMin = 1000.0;
        double forceMid = 4000.0;
        double forceMax = 8000.0;

        // rescale curved log output [0, log1p(10)] to [1000, 8000]
        double maxLog = std::log1p(10.0);
        force = forceMin + (curved / maxLog) * (forceMax - forceMin);
    }

    // Step 1.5: Apply slip angle reduction
    
    // This is all about adding feeling by augmenting the basic force as the car slides
    //New slip angle v0.7 Alpha
    //26% reduction at 20degree slip
    double slipWeight = slip.absSlipDeg;
    double gWeight = std::abs(g);
    double slipInfluenceScale = std::clamp(1.0 - (gWeight / 6.0), 0.25, 1.0);
    double slipReduction = std::clamp((slipWeight / 20.0) * 0.40 * slipInfluenceScale, 0.0, 1.0);
    force *= (1.0 - slipReduction);

    // Old slip logic
    //double slipReduction = std::clamp(slip.absSlipDeg / 20.0 * 0.40, 0.0, 1.0);  // max 100% reduction
    //force *= (1.0 - slipReduction);

    // Normalize and preserve sign\
    // v0.6 Alpha added inversion capability
    bool invert = (targetInvertFFB == L"true" || targetInvertFFB == L"True");

    double curved = (force / 10000.0) * (
        (g < 0.0 ? -1.0 : 1.0) * (invert ? -1.0 : 1.0)
        );

    // Step 2: Smoothing (moving average)
    static std::deque<double> forceHistory;
    forceHistory.push_back(curved);
    if (forceHistory.size() > 3) {
        forceHistory.pop_front();
    }
    double smoothed = std::accumulate(forceHistory.begin(), forceHistory.end(), 0.0) / forceHistory.size();
    int magnitude = static_cast<int>(std::abs(smoothed) * 10000.0 * masterForceScale);

    // can reverse effect here
    //LONG targetDir = (curved > 0.0 ? -1 : (curved < 0.0 ? 1 : 0)) * 10000;
    LONG targetDir = (curved > 0.0 ? -10000 : (curved < 0.0 ? 10000 : 0));


    static LONG lastDirection = 0;
    constexpr double directionSmoothingFactor = 0.2;
    lastDirection = static_cast<LONG>(
        (1.0 - directionSmoothingFactor) * lastDirection +
        directionSmoothingFactor * targetDir
        );

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