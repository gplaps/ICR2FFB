#include "constant_force.h"
#include "constants.h"
#include "helpers.h"
#include "log.h"
#include "main.h"
#include <iostream>
#include <algorithm>
#include <deque>
#include <numeric>
#include <cmath>

/*
 * Copyright 2025 gplaps
 *
 * Licensed under the MIT License (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/MIT
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */



static DIEFFECT CreateConstantForceEffect(LONG magnitude) {
    DICONSTANTFORCE cf = { magnitude };
    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwGain = DEFAULT_DINPUT_GAIN;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.cAxes = 1;
    DWORD axes[1] = { DIJOFS_X };
    LONG dir[1] = { 0 }; // ← Always zero direction now, this broke Moza wheels
    eff.rgdwAxes = axes;
    eff.rglDirection = dir;
    eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;
    return eff;
}

void ApplyConstantForceEffect(const RawTelemetry& current,
    const CalculatedLateralLoad& /*load*/, const CalculatedSlip& /*slip*/,
    const CalculatedVehicleDynamics& vehicleDynamics,
    double speed_mph, double /*steering_deg*/, IDirectInputEffect* effect,
    bool /*enableWeightForce*/,
    bool enableRateLimit,
    double masterForceScale,
    double deadzoneForceScale,
    double constantForceScale,
    double /*weightForceScale*/,
    bool invert
    ) {

    if (!effect) return;

    // Beta 0.5
    // This is a bunch of logic to pause/unpause or prevent forces when the game isn't running
    // I think this could probably be a lot simpler so maybe up for a redo
    // It works right now though, although it feels a bit delayed

    static double lastDlong = 0.0;
    static bool hasEverMoved = false; (void)hasEverMoved; // unused currently
    static int noMovementFrames = 0;
    static bool isPaused = true;
    static bool pauseForceSet = false;
    static bool isFirstReading = true;
    const int movementThreshold = 10;  // Frames to consider "paused"
    const double movementThreshold_value = 0.001;  // Very small movement threshold

    if (isFirstReading) {
        lastDlong = current.dlong;  // Set baseline from first real data
        isFirstReading = false;
        if (!pauseForceSet) {
            DIEFFECT eff = CreateConstantForceEffect(0);
            effect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS | DIEP_DIRECTION);
            pauseForceSet = true;
        }
        return;
    }

    bool isStationary = std::abs(current.dlong - lastDlong) < movementThreshold_value;

    if (isStationary) {
        noMovementFrames++;
        if (noMovementFrames >= movementThreshold && !isPaused) {
            isPaused = true;
            pauseForceSet = false;  // ← Reset when entering pause
            LogMessage(L"[INFO] Game paused detected - sending zero force");
        }
    }
    else {
        if (isPaused || noMovementFrames > 0) {
            noMovementFrames = 0;
            if (isPaused) {
                isPaused = false;
                pauseForceSet = false;  // ← Reset when exiting pause
                LogMessage(L"[INFO] Game resumed - restoring normal forces");
            }
        }
        lastDlong = current.dlong;
    }

    // If paused, send zero force and return
    if (isPaused) {
        if (!pauseForceSet) {
            DIEFFECT eff = CreateConstantForceEffect(0);
            effect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS | DIEP_DIRECTION);
            pauseForceSet = true;
        }
        return;
    }

    // Low speed filtering
    if (speed_mph < 5.0) {
        static bool wasLowSpeed = false;
        if (!wasLowSpeed) {
            // Send zero force when entering low speed
            DIEFFECT eff = CreateConstantForceEffect(0);
            effect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS | DIEP_DIRECTION);
            wasLowSpeed = true;
        }
        return;
    }

    static bool wasLowSpeed = false;
    if (wasLowSpeed) {
        wasLowSpeed = false;  // Reset when speed picks up
    }


// === CALC 1 - Lateral G ===

// This will add a 'base' force based on speed. This helps INCREDIBLY with straight line control
// It gets rid of that 'ping pong' effect that I've been chasing while not making the forces feel delayed
/*
    double baseLoad = 0.0;
    if (speed_mph > 60.0) {
        double speedFactor = (speed_mph - 60.0) / 180.0;  // 0.0 at 120mph, 1.0 at 220mph
        if (speedFactor > 1.0) speedFactor = 1.0;
        baseLoad = speedFactor * speedFactor * 1200.0;    // Amount of force to apply at 'peak'
    }

    // === G-Force cornering ===
    double absG = std::abs(vehicleDynamics.lateralG);
    double corneringForce = 0.0;

    double configuredDeadzone = 0.03 * deadzoneForceScale;  // Version 0.8 Beta added Deadzone to ini

    if (absG > configuredDeadzone) {
        double effectiveG = absG - configuredDeadzone;

        // Adjust the calculation range based on deadzone
        double maxEffectiveG = 4.0 - configuredDeadzone;

        if (effectiveG <= maxEffectiveG) {
            double normalizedG = effectiveG / maxEffectiveG;
            double curveValue = normalizedG * (0.7 + 0.3 * normalizedG);
            corneringForce = curveValue * 8000.0;
        }
        else {
            corneringForce = 8000.0;
        }
    }

    // === Final force calculation ===
    double force;

    if (vehicleDynamics.lateralG > 0.03) {
        // Right turn
        force = baseLoad + corneringForce;
    }
    else if (vehicleDynamics.lateralG < -0.03) {
        // Left turn
        force = baseLoad + corneringForce;
    }
    else {
        // Straight line - very light
        force = baseLoad * 0.2;  // Even lighter when straight
    }
*/

// Niels Equation
// Calculate based on front tire load directly instead of linear G = Much better
// No issues with oscillations anymore

// Get the sum with signs preserved
    double frontTireLoadSum = vehicleDynamics.frontLeftForce_N + vehicleDynamics.frontRightForce_N;

    // Use the magnitude for physics calculation
    double frontTireLoadMagnitude = std::abs(frontTireLoadSum);

    // Keep frontTireLoad for logging compatibility
    double frontTireLoad = frontTireLoadMagnitude;

    // Physics constants from your engineer friend
    const double CURVE_STEEPNESS = 1.0e-4;
    const double MAX_THEORETICAL = 8500;
    const double SCALE_FACTOR = 1.20;

    // Calculate magnitude using physics formula
    double physicsForceMagnitude = atan(frontTireLoadMagnitude * CURVE_STEEPNESS) * MAX_THEORETICAL * SCALE_FACTOR;

    // Apply the natural sign from the tire load sum
    double physicsForce = (frontTireLoadSum >= 0) ? physicsForceMagnitude : -physicsForceMagnitude;

    // Apply proportional deadzone
    double force = physicsForce;
    if (deadzoneForceScale > 0.0) {
        // Convert deadzoneForceScale (0-100) to percentage (0.0-1.0)
        double deadzonePercentage = deadzoneForceScale / 100.0;

        // Calculate deadzone threshold using magnitude
        double maxPossibleForce = MAX_THEORETICAL * SCALE_FACTOR; // ~10200
        double deadzoneThreshold = maxPossibleForce * deadzonePercentage;

        // Apply deadzone: remove bottom X% and rescale remaining range
        if (std::abs(physicsForce) <= deadzoneThreshold) {
            force = 0.0;  // Force is in deadzone - zero output
        }
        else {
            // Rescale remaining force range to maintain full output range
            double remainingRange = maxPossibleForce - deadzoneThreshold;
            double adjustedInput = std::abs(physicsForce) - deadzoneThreshold;
            double scaledMagnitude = (adjustedInput / remainingRange) * maxPossibleForce;

            // Restore original sign
            force = (physicsForce >= 0) ? scaledMagnitude : -scaledMagnitude;
        }
    }

    // Cap maximum force magnitude while preserving sign
    if (std::abs(force) > 10000.0) {
        force = (force >= 0) ? 10000.0 : -10000.0;
    }

    // Handle invert option (no more complex direction logic needed!)
    if (invert) {
        force = -force;
    }

    // Convert to signed magnitude
    double smoothed = force;
    int magnitude = static_cast<int>(std::abs(smoothed) * masterForceScale * constantForceScale);
    int signedMagnitude = static_cast<int>(magnitude);
    if (smoothed < 0.0) {
        signedMagnitude = -signedMagnitude;
    }



// === Self-Aligning Torque ===
/*
    const double STEERING_RATIO = 15.0;
    double wheel_angle_deg = current.steering_deg / STEERING_RATIO;
    double wheel_angle_rad = wheel_angle_deg * 3.14159 / 180.0;
    double steeringAngleDegrees = wheel_angle_rad * (180.0 / 3.14159);

    if (speed_mph > 5.0) {
        double speedFactor = std::clamp(speed_mph / 60.0, 0.2, 1.2);
        double absAngle = std::abs(steeringAngleDegrees);

        // Add proper dead zone for straight-line stability
        const double steeringDeadZone = 2.0;  // 2 degrees dead zone

        if (absAngle > steeringDeadZone) {  // Only apply SAT when actually steering
            // Reduce effective angle by dead zone
            double effectiveAngle = absAngle - steeringDeadZone;

            // 1. Self-aligning torque (wants to center) - reduced strength
            double satForce = speedFactor * (effectiveAngle / 25.0) * 400.0;  // Reduced from 800

            // Always opposes current steering direction
            if (steeringAngleDegrees > 0) {
                signedMagnitude -= static_cast<int>(satForce * 0.5);  // Reduced from 0.7
            }
            else {
                signedMagnitude += static_cast<int>(satForce * 0.5);
            }

            // 2. Steering resistance (makes turning harder) - only for larger angles
            if (effectiveAngle > 3.0) {  // Only add resistance for larger steering inputs
                double resistanceForce = effectiveAngle * speedFactor * 8.0;  // Reduced from 15

                if (signedMagnitude > 0) {
                    signedMagnitude += static_cast<int>(resistanceForce);
                }
                else if (signedMagnitude < 0) {
                    signedMagnitude -= static_cast<int>(resistanceForce);
                }
            }
        }
    }
*/
// === CALC 2 - Slip ===

    // This should modify the forces based on the amount of slip angle the car has
    // The linear force feels good on its own, but this can add some more detail
/*
    if (std::abs(vehicleDynamics.slip) > 0.05) {  // Only apply if meaningful slip

        // Calculate slip modifier: 5% change per 2 degrees
        double slipDegrees = std::abs(vehicleDynamics.slip);
        double slipModifier = (slipDegrees / 2.0) * 0.05;

        // Cap the modifier to reasonable limits (max 60% change)
        slipModifier = std::clamp(slipModifier, 0.0, 0.60);

        bool isLeftTurn = (vehicleDynamics.lateralG < 0.0);
        bool isUndersteer;

        if (isLeftTurn) {
            // Left turn: positive slip = oversteer, negative slip = understeer
            isUndersteer = (vehicleDynamics.slip < 0);
        }
        else {
            // Right turn: negative slip = oversteer, positive slip = understeer  
            isUndersteer = (vehicleDynamics.slip > 0);
        }

        if (isUndersteer) {
            // UNDERSTEER: INCREASE force (heavier steering)
            signedMagnitude = static_cast<int>(signedMagnitude * (1.0 + slipModifier));
        }
        else {
            // OVERSTEER: DECREASE force (lighter steering)
            signedMagnitude = static_cast<int>(signedMagnitude * (1.0 - slipModifier));
        }

        // Ensure we don't go negative due to oversteer reduction
        if (smoothed < 0.0 && signedMagnitude > 0) {
            signedMagnitude = -signedMagnitude;  // Restore correct sign
        }
        else if (smoothed > 0.0 && signedMagnitude < 0) {
            signedMagnitude = -signedMagnitude;  // Restore correct sign
        }
    }
*/

// === Speed-based Centering ===
/*
// Damping that opposes any steering input
    if (speed_mph > 10.0 && std::abs(vehicleDynamics.lateralG) < 0.15) {
        // Add baseline steering resistance at speed (not centering force)
        double speedResistance = std::clamp(speed_mph / 80.0, 0.0, 1.0) * 300.0;

        // Only add resistance if current force is very small
        if (std::abs(signedMagnitude) < speedResistance) {
            // Preserve direction but ensure minimum resistance
            if (signedMagnitude >= 0) {
                signedMagnitude = static_cast<int>(speedResistance);
            }
            else {
                signedMagnitude = -static_cast<int>(speedResistance);
            }
        }
    }
*/
// === Output Smoothing ===

// Take final magnitude and prevent any massive jumps over a small frame range

    static std::deque<int> magnitudeHistory;
    magnitudeHistory.push_back(signedMagnitude);
    if (magnitudeHistory.size() > 2) {
        magnitudeHistory.pop_front();
    }
    signedMagnitude = std::accumulate(magnitudeHistory.begin(), magnitudeHistory.end(), 0) / static_cast<int>(magnitudeHistory.size());
 

    // === CALC 3 Weight Force ===
        // Weight shifting code
        // This will try to create some feeling based on the front tire loads changing to hopefully 'feel' the road more
        // It watches for changes in the left to right split of force
        // 
        // If the front left tire is doing 45% of the work and right front is doing 55%
        // And then they change and the left is doing 40% and the right is 60%
        // You will feel a bump
        // This adds detail to camber and surface changes
    /*
    if (enableWeightForce && speed_mph > 1.0) {
        static double lastFrontImbalance = 0.0;
        static double weightTransferForce = 0.0;
        static int framesSinceChange = 0;

        double totalFrontLoad = vehicleDynamics.frontLeftForce_N + vehicleDynamics.frontRightForce_N;
        if (totalFrontLoad > 100.0) {
            double currentImbalance = (vehicleDynamics.frontLeftForce_N - vehicleDynamics.frontRightForce_N) / totalFrontLoad;
            double imbalanceChange = currentImbalance - lastFrontImbalance;

            
            if (std::abs(imbalanceChange) > 0.005) {  // How much of a change to consider between left/right split
                // Much stronger force
                weightTransferForce += imbalanceChange * 4500.0;  // Force scaler
                framesSinceChange = 0;

                // Higher cap for stronger effects
                weightTransferForce = std::clamp(weightTransferForce, -5000.0, 5000.0);
            }
            else {
                framesSinceChange++;
            }

            // Decay time to hold effect, don't want to hold it forever because then we dont feel regular force
            if (framesSinceChange > 60) { 
                weightTransferForce *= 0.995;  // decay time per frame, inverse so 0.005% per frame

                if (std::abs(weightTransferForce) < 20.0) {
                    weightTransferForce = 0.0;
                }
            }

            signedMagnitude += static_cast<int>((weightTransferForce * masterForceScale) * weightForceScale);
            lastFrontImbalance = currentImbalance;
        }
    }
    */

/*
// === Reduce update Rate ===
// 
    // This will reduce the rate at which we send updates to the wheel
    // The idea is to not break older controllers or ones which can't take frequent updates
    // The risk is adding delay


    static int lastSentMagnitude = -1;
    static int lastSentSignedMagnitude = 0;
    static int lastProcessedMagnitude = -1;
    static int framesSinceLastUpdate = 0;
    static double accumulatedMagnitudeChange = 0.0;
    static double accumulatedSignChange = 0.0;


    if (lastSentMagnitude != -1) {
        accumulatedMagnitudeChange += std::abs(magnitude - lastProcessedMagnitude);
        accumulatedSignChange += std::abs(signedMagnitude - lastSentSignedMagnitude);
    }

    framesSinceLastUpdate++;
    bool shouldUpdate = false;

    // 1. More sensitive immediate changes
    if (std::abs(magnitude - lastSentMagnitude) >= 400 ||     // Reduced from 400
        std::abs(signedMagnitude - lastSentSignedMagnitude) >= 2000) { // Reduced from 800
        shouldUpdate = true;
    }
    // 2. More sensitive accumulated changes  
    else if (accumulatedMagnitudeChange >= 300 ||             // Reduced from 300
        accumulatedSignChange >= 1500) {                   // Reduced from 600
        shouldUpdate = true;
    }
    // 3. Direction change (unchanged - always important)
    else if ((lastSentSignedMagnitude > 0 && signedMagnitude < 0) ||
        (lastSentSignedMagnitude < 0 && signedMagnitude > 0) ||
        (lastSentSignedMagnitude == 0 && signedMagnitude != 0) ||
        (lastSentSignedMagnitude != 0 && signedMagnitude == 0)) {
        shouldUpdate = true;
    }
    // 4. Timeout, how long until we send an update no matter what
    else if (framesSinceLastUpdate >= 12) {                    // Reduced from 12 (~15Hz)
        shouldUpdate = true;
    }
    // 5. Zero force (unchanged)
    else if (magnitude == 0 && lastSentMagnitude != 0) {
        shouldUpdate = true;
    }

    if (!shouldUpdate) {
        lastProcessedMagnitude = magnitude;
        return;
    }

    // Reset tracking
    lastSentMagnitude = magnitude;
    lastSentSignedMagnitude = signedMagnitude;
    framesSinceLastUpdate = 0;
    accumulatedMagnitudeChange = 0.0;
    accumulatedSignChange = 0.0;
    lastProcessedMagnitude = magnitude;
    
    */

    // === Rate Limiting ===
// === Reimplemnted older style to try to make compatible with Thrustmaster wheels ===
    if (enableRateLimit) {
        // Direction calculation and smoothing for rate limiting
        LONG targetDir = -sign(smoothed) * static_cast<LONG>(DEFAULT_DINPUT_GAIN);
        static LONG lastDirection = 0;

        // Direction smoothing - this prevents rapid direction changes
        const double directionSmoothingFactor = 0.3;
        lastDirection = static_cast<LONG>((1.0 - directionSmoothingFactor) * lastDirection + directionSmoothingFactor * targetDir);

        // Rate limiting with direction smoothing
        static int lastSentMagnitude = -1;
        static int lastSentSignedMagnitude = 0; (void) lastSentSignedMagnitude; // unused currently
        static LONG lastSentDirection = 0;  // Track smoothed direction
        static int lastProcessedMagnitude = -1;
        static int framesSinceLastUpdate = 0;
        static double accumulatedMagnitudeChange = 0.0;
        static double accumulatedDirectionChange = 0.0;  // Track direction changes

        // Track accumulated changes since last update
        if (lastSentMagnitude != -1) {
            accumulatedMagnitudeChange += std::abs(magnitude - lastProcessedMagnitude);
            accumulatedDirectionChange += std::abs(lastDirection - lastSentDirection);  // Use smoothed direction
        }

        framesSinceLastUpdate++;
        bool shouldUpdate = false;

        // 1. Large immediate change
        if (std::abs(magnitude - lastSentMagnitude) >= 400 ||
            std::abs(lastDirection - lastSentDirection) >= 2000) {  // Use smoothed direction
            shouldUpdate = true;
        }
        // 2. Accumulated changes
        else if (accumulatedMagnitudeChange >= 300 ||
            accumulatedDirectionChange >= 1500) {  // Use direction accumulation
            shouldUpdate = true;
        }
        // 3. Direction sign change (use smoothed direction)
        else if ((lastSentDirection > 0 && lastDirection < 0) ||
            (lastSentDirection < 0 && lastDirection > 0) ||
            (lastSentDirection == 0 && lastDirection != 0) ||
            (lastSentDirection != 0 && lastDirection == 0)) {
            shouldUpdate = true;
        }
        // 4. Timeout
        else if (framesSinceLastUpdate >= 12) {
            shouldUpdate = true;
        }
        // 5. Zero force
        else if (magnitude == 0 && lastSentMagnitude != 0) {
            shouldUpdate = true;
        }

        if (!shouldUpdate) {
            lastProcessedMagnitude = magnitude;
            return;  // Skip this frame
        }

        // Reset tracking when we send an update
        lastSentMagnitude = magnitude;
        lastSentSignedMagnitude = signedMagnitude;
        lastSentDirection = lastDirection;  // Track the smoothed direction
        framesSinceLastUpdate = 0;
        accumulatedMagnitudeChange = 0.0;
        accumulatedDirectionChange = 0.0;
        lastProcessedMagnitude = magnitude;
    }

    g_currentFFBForce = signedMagnitude;

    //Logging
    static int debugCounter = 0;
    if (debugCounter % 30 == 0) {  // Every 30 frames
        LogMessage(L"[DEBUG] FL: " + std::to_wstring(vehicleDynamics.frontLeftForce_N) +
            L", FR: " + std::to_wstring(vehicleDynamics.frontRightForce_N) +
            L", Total: " + std::to_wstring(frontTireLoad) +
            L", atan_input: " + std::to_wstring(frontTireLoad * 1.0e-4) +
            L", atan_result: " + std::to_wstring(atan(frontTireLoad * 1.0e-4)));
    }
    debugCounter++;

    DIEFFECT eff = CreateConstantForceEffect(signedMagnitude); // Use signed magnitude
    // Only set magnitude params, skip direction
    HRESULT hr = effect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);  // ← Removed | DIEP_DIRECTION

    if (FAILED(hr)) {
        std::wcerr << L"Constant force SetParameters failed: 0x" << std::hex << hr << std::endl;
    }
}
