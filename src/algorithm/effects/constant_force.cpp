#include "constant_force.h"

#include "constants.h"
#include "log.h"
#include "math_utilities.h"
#include "string_utilities.h" // IWYU pragma: keep

#include <cmath>
#include <numeric>

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

double ConstantForceEffect::ApplyDeadzone(double physicsForce, double deadzoneForceScale)
{
    double force = physicsForce;

    if (deadzoneForceScale > 0.0)
    {
        // Calculate deadzone threshold using magnitude
        const double maxPossibleForce  = MAX_FORCE_IN_N; // ~10200
        const double deadzoneThreshold = maxPossibleForce * deadzoneForceScale;

        // Apply deadzone: remove bottom X% and rescale remaining range
        if (std::abs(physicsForce) <= deadzoneThreshold)
        {
            force = 0.0; // Force is in deadzone - zero output
        }
        else
        {
            // Rescale remaining force range to maintain full output range
            const double remainingRange  = maxPossibleForce - deadzoneThreshold;
            const double adjustedInput   = std::abs(physicsForce) - deadzoneThreshold;
            const double scaledMagnitude = (adjustedInput / remainingRange) * maxPossibleForce;

            // Restore original sign
            force = (physicsForce >= 0) ? scaledMagnitude : -scaledMagnitude;
        }
    }
    return force;
}

// Take final magnitude and prevent any massive jumps over a small frame range
int ConstantForceEffect::SmoothSpikes(int signedMagnitude)
{
    magnitudeHistory.push_back(signedMagnitude);
    while (magnitudeHistory.size() > 2)
    {
        magnitudeHistory.pop_front();
    }
    signedMagnitude = std::accumulate(magnitudeHistory.begin(), magnitudeHistory.end(), 0) / static_cast<int>(magnitudeHistory.size());
    return signedMagnitude;
}

ConstantForceEffectResult ConstantForceEffect::Calculate(const RawTelemetry& current,
                                                         const CalculatedLateralLoad& /*load*/,
                                                         const CalculatedSlip& /*slip*/,
                                                         const CalculatedVehicleDynamics& vehicleDynamics,
                                                         bool                             enableRateLimit,
                                                         double                           deadzoneForceScale,
                                                         double                           brakingForceScale,
                                                         double /*weightForceScale*/)
{
    const double speed_mph = current.speed_mph;
    (void)speed_mph; // used in commented code - be aware of spinning / reversing with negative speed, e.g. std::abs(speed_mph)
    // const double steering_deg = current.steering_deg;

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
    //double frontTireLoadSum = vehicleDynamics.frontLeftForce_N + vehicleDynamics.frontRightForce_N; //original lat forces only

    const double longScaler       = 4.0 * brakingForceScale;

    const double frontTireLongSum = (vehicleDynamics.frontRightLong_N - vehicleDynamics.frontLeftLong_N) * longScaler;

    const double frontTireLoadSum = (vehicleDynamics.frontLeftForce_N + vehicleDynamics.frontRightForce_N) + frontTireLongSum;


    // double frontTireLoadSum = (vehicleDynamics.frontLeftForce_N + (vehicleDynamics.frontLeftLong_N * longScaler)) + (vehicleDynamics.frontRightForce_N + (vehicleDynamics.frontRightLong_N * longScaler));

    // Use the magnitude for physics calculation
    const double frontTireLoadMagnitude = std::abs(frontTireLoadSum);

    // Keep frontTireLoad for logging compatibility
    const double frontTireLoad = frontTireLoadMagnitude;

    // Calculate magnitude using physics formula
    const double physicsForceMagnitude = atan(frontTireLoadMagnitude * CURVE_STEEPNESS) * MAX_FORCE_IN_N;

    // Apply the natural sign from the tire load sum
    const double physicsForce = (frontTireLoadSum >= 0) ? physicsForceMagnitude : -physicsForceMagnitude;

    // Apply proportional deadzone
    double force = ApplyDeadzone(physicsForce, deadzoneForceScale);

    // Cap maximum force magnitude while preserving sign
    force = std::clamp(force, -MAX_FORCE_IN_N, MAX_FORCE_IN_N);

    // Convert to signed magnitude
    const double smoothed        = force;
    int          signedMagnitude = static_cast<int>(smoothed);
    // int magnitude      = std::abs(signedMagnitude);
    // (void)magnitude;


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
        double speedResistance = saturate(speed_mph / 80.0) * 300.0;

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
    signedMagnitude = SmoothSpikes(signedMagnitude);


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

            signedMagnitude += static_cast<int>((weightTransferForce) * weightForceScale);
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
    if (enableRateLimit)
    {
        signedMagnitude = rateLimiter.Calculate(signedMagnitude, smoothed);
    }

    //Logging
    static int debugCounter = 0;
    if (debugCounter % 30 == 0)
    { // Every 30 frames
        LogMessage(L"[DEBUG] FL: " + std::to_wstring(vehicleDynamics.frontLeftForce_N) +
                   L", FR: " + std::to_wstring(vehicleDynamics.frontRightForce_N) +
                   L", Total: " + std::to_wstring(frontTireLoad) +
                   L", atan_input: " + std::to_wstring(frontTireLoad * 1.0e-4) +
                   L", atan_result: " + std::to_wstring(atan(frontTireLoad * 1.0e-4)));
    }
    debugCounter++;

    // Use signed magnitude
    // Only set magnitude params, skip direction
    return ConstantForceEffectResult(static_cast<double>(signedMagnitude) / MAX_FORCE_IN_N /* or is this DEFAULT_DINPUT_GAIN ? */, false);
}
