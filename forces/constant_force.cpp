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

    // Low speed filtering
    if (speed_mph < 5.0) {
        static bool wasLowSpeed = false;
        if (!wasLowSpeed) {
            // Send zero force when entering low speed
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
            wasLowSpeed = true;
        }
        return;
    }

    static bool wasLowSpeed = false;
    if (wasLowSpeed) {
        wasLowSpeed = false;  // Reset when speed picks up
    }


    // Paused detection to make forces go away when the car isnt moving
    // Should be safe for all devices because its not stopping the effect, just making it 0

    static double lastDlong = 0.0;
    static int noMovementFrames = 0;
    static bool isPaused = false;
    constexpr int movementThreshold = 10;  // Frames to consider "paused"
    constexpr double movementThreshold_value = 0.001;  // Very small movement threshold

    bool isStationary = std::abs(current.dlong - lastDlong) < movementThreshold_value;

    if (isStationary) {
        noMovementFrames++;
        if (noMovementFrames >= movementThreshold && !isPaused) {
            isPaused = true;
            LogMessage(L"[INFO] Game paused detected - sending zero force");
        }
    }
    else {
        if (isPaused || noMovementFrames > 0) {
            noMovementFrames = 0;
            if (isPaused) {
                isPaused = false;
                LogMessage(L"[INFO] Game resumed - restoring normal forces");
            }
        }
        lastDlong = current.dlong;
    }

    // If paused, send zero force and return
    if (isPaused) {
        static bool pauseForceSet = false;
        if (!pauseForceSet) {
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
            pauseForceSet = true;
        }
        return;
    }

    static bool pauseForceSet = false;
    if (pauseForceSet) {
        pauseForceSet = false;  // Reset when unpaused
    }


    // MAIN EVENT !!!!!!
    // Force Calculations

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
        (g < 0.0 ? -1.0 : 1.0) * (invert ? 1.0 : -1.0)
        );

    // Step 2: Smoothing (moving average)
    static std::deque<double> forceHistory;
    forceHistory.push_back(curved);
    if (forceHistory.size() > 3) {
        forceHistory.pop_front();
    }
    double smoothed = std::accumulate(forceHistory.begin(), forceHistory.end(), 0.0) / forceHistory.size();
    int magnitude = static_cast<int>(std::abs(smoothed) * 10000.0 * masterForceScale);

    // How to calculate the direction
    // can reverse effect here
   // LONG targetDir = (curved > 0.0 ? -1 : (curved < 0.0 ? 1 : 0)) * 10000;
    LONG targetDir = (smoothed > 0.0 ? -1 : (smoothed < 0.0 ? 1 : 0)) * 10000;


    // Newer logic but might have broken older wheels?
    //LONG targetDir = (curved > 0.0 ? -10000 : (curved < 0.0 ? 10000 : 0));

    static LONG lastDirection = 0;


    // Ultra-slow single smoothing 
    constexpr double directionSmoothingFactor = 0.3; // Instead of 0.2
    lastDirection = static_cast<LONG>((1.0 - directionSmoothingFactor) * lastDirection + directionSmoothingFactor * targetDir);

    // Blend direction near center to avoid sharp swaps
    // Beta 0.3 removing because duplicate?
    //lastDirection = static_cast<LONG>((1.0 - directionSmoothingFactor) * lastDirection + directionSmoothingFactor * targetDir);

    // Step 4: Avoid micro-changes
    // Change with Beta 3 to prevent overwhelming older wheels
    static int lastSentMagnitude = -1;
    static LONG lastSentDirection = 0;
    static int lastProcessedMagnitude = -1;  // Track previous frame's magnitude
    static int framesSinceLastUpdate = 0;
    static double accumulatedMagnitudeChange = 0.0;
    static double accumulatedDirectionChange = 0.0;

    // Track accumulated changes since last update
    if (lastSentMagnitude != -1) {
        accumulatedMagnitudeChange += std::abs(magnitude - lastProcessedMagnitude);
        accumulatedDirectionChange += std::abs(lastDirection - lastSentDirection);
    }

    framesSinceLastUpdate++;

    // Conditions for sending an update
    bool shouldUpdate = false;
    std::wstring updateReason;

    // 1. Large immediate change
    if (std::abs(magnitude - lastSentMagnitude) >= 400 || std::abs(lastDirection - lastSentDirection) >= 2000) {
        shouldUpdate = true;
        updateReason = L"Large immediate change";
    }
    // 2. Accumulated drift is significant
    else if (accumulatedMagnitudeChange >= 300 || accumulatedDirectionChange >= 1500) {
        shouldUpdate = true;
        updateReason = L"Accumulated change";
    }
    // 3. Direction sign change (always important)
    else if ((lastSentDirection > 0 && lastDirection < 0) || (lastSentDirection < 0 && lastDirection > 0) ||
        (lastSentDirection == 0 && lastDirection != 0) || (lastSentDirection != 0 && lastDirection == 0)) {
        shouldUpdate = true;
        updateReason = L"Direction change";
    }
    // 4. Force timeout - prevent wheels from "forgetting" current force
    // Change this to make the maximum "Wait" time larger between forces
    else if (framesSinceLastUpdate >= 12) {  // should be ~10hz?
        shouldUpdate = true;
        updateReason = L"Timeout refresh";
    }
    // 5. Zero force (always send immediately for responsiveness)
    else if (magnitude == 0 && lastSentMagnitude != 0) {
        shouldUpdate = true;
        updateReason = L"Zero force";
    }

    // Skip update if no meaningful change
    if (!shouldUpdate) {
        lastProcessedMagnitude = magnitude;  // Track for next frame calculation
        return;
    }

    // Reset tracking when we send an update
    lastSentMagnitude = magnitude;
    lastSentDirection = lastDirection;
    framesSinceLastUpdate = 0;
    accumulatedMagnitudeChange = 0.0;
    accumulatedDirectionChange = 0.0;
    lastProcessedMagnitude = magnitude;

    // Optional: Log update frequency for debugging
    static int updateCount = 0;
    updateCount++;
    if (updateCount % 100 == 0) {
        LogMessage(L"[DEBUG] FFB updates sent: " + std::to_wstring(updateCount) + L", reason: " + updateReason);
    }



    //Step 5 Apply Affect

    // Encode direction in magnitude sign instead of separate direction
    int signedMagnitude = magnitude;
    if (smoothed < 0.0) {  // Right turn (negative smoothed = right)
        signedMagnitude = -static_cast<int>(magnitude);  // Negative magnitude for right
    }
    // Left turn (positive smoothed = left) keeps positive magnitude

    DICONSTANTFORCE cf = { signedMagnitude };  // Use signed magnitude
    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwGain = 10000;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.cAxes = 1;
    DWORD axes[1] = { DIJOFS_X };
    LONG dir[1] = { 0 };  // ← Always zero direction now
    eff.rgdwAxes = axes;
    eff.rglDirection = dir;
    eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;

    // Only set magnitude params, skip direction
    HRESULT hr = constantForceEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);  // ← Removed | DIEP_DIRECTION

    if (FAILED(hr)) {
        std::wcerr << L"Constant force SetParameters failed: 0x" << std::hex << hr << std::endl;
    }

    // Optional: Add debug logging to see what values are being sent
    static int debugCounter = 0;
    debugCounter++;
    if (debugCounter % 60 == 0) {  // Every second
        LogMessage(L"[MOZA TEST] LateralG=" + std::to_wstring(load.lateralG) +
            L", SignedMag=" + std::to_wstring(signedMagnitude) +
            L", Smoothed=" + std::to_wstring(smoothed));
    }
}