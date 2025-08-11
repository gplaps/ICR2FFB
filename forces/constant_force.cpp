#include "constant_force.h"
#include <iostream>
#include <algorithm>
#include <deque>
#include <numeric>




// Get config data
extern std::wstring targetInvertFFB;

// To be used in reporting
extern int g_currentFFBForce;

void ApplyConstantForceEffect(const RawTelemetry& current,
    const CalculatedLateralLoad& load, const CalculatedSlip& slip,
    const CalculatedVehicleDynamics& vehicleDynamics,
    double speed_mph, double steering_deg, IDirectInputEffect* constantForceEffect,
    double masterForceScale) {

    if (!constantForceEffect) return;

    // Beta 0.5
    // This is a bunch of logic to pause/unpause or prevent forces when the game isn't running
    // I think this could probably be a lot simpler so maybe up for a redo
    // It works right now though, although it feels a bit delayed

    static double lastDlong = 0.0;
    static bool hasEverMoved = false;
    static int noMovementFrames = 0;
    static bool isPaused = true;
    static bool pauseForceSet = false;
    static bool isFirstReading = true;
    constexpr int movementThreshold = 10;  // Frames to consider "paused"
    constexpr double movementThreshold_value = 0.001;  // Very small movement threshold

    if (isFirstReading) {
        lastDlong = current.dlong;  // Set baseline from first real data
        isFirstReading = false;
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


// === CALC 1 - Lateral G ===

/*
    // Linear force applied to G
    // 1G = 1500 or ~15% of a wheels total force
    // 4G (indy) would be 60% of the wheels force which feels nice
    double force = std::abs(vehicleDynamics.lateralG) * 1500.0;

    //Logic to reverse force direction if needed based on ini
    bool invert = (targetInvertFFB == L"true" || targetInvertFFB == L"True");
    double directionMultiplier = (vehicleDynamics.lateralG > 0 ? 1.0 : -1.0);
    if (invert) {
        directionMultiplier = -directionMultiplier;
    }

    double smoothed = force * directionMultiplier;
    int magnitude = static_cast<int>(std::abs(smoothed) * masterForceScale);

    int signedMagnitude = static_cast<int>(magnitude);
    if (smoothed < 0.0) {
        signedMagnitude = -signedMagnitude;
    }
*/

// Curved force applied, this feels better because most corners are ~1 - 2 G
// But using a force that is high enough for those on linear makes ovals too strong
// Target: 1000 force at 0.5G, 6000 force at 4G (same as linear: 4G × 1500 = 6000)
    double absG = std::abs(vehicleDynamics.lateralG);
    double force;

    const double centerDeadZone = 0.08;

    if (absG <= centerDeadZone) {
        // Linear ramp from 0 to target force at dead zone edge
        force = (absG / centerDeadZone) * 1200.0;  // Linear from 0 to 800 force units
    }
    else {
        // Your existing force calculation above dead zone
        double normalizedG = absG / 4.0;
        double logValue = std::log1p(normalizedG * 15.0);
        double maxLogValue = std::log1p(15.0);
        force = (logValue / maxLogValue) * 6000.0;

    }

    // Cap maximum force 
    if (force > 10000.0) {
        force = 10000.0;
    }   

    // Logic to reverse force direction if needed based on ini
    bool invert = (targetInvertFFB == L"true" || targetInvertFFB == L"True");
    double directionMultiplier = (vehicleDynamics.lateralG > 0 ? 1.0 : -1.0);
    if (invert) {
        directionMultiplier = -directionMultiplier;
    }

    double smoothed = force * directionMultiplier;
    int magnitude = static_cast<int>(std::abs(smoothed) * masterForceScale);
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
// === Smoothing Code ===

    static std::deque<int> magnitudeHistory;
    magnitudeHistory.push_back(signedMagnitude);
    if (magnitudeHistory.size() > 3) {
        magnitudeHistory.pop_front();
    }
    signedMagnitude = static_cast<int>(std::accumulate(magnitudeHistory.begin(), magnitudeHistory.end(), 0.0) / magnitudeHistory.size());
 

    // === Experimental ===
        // Weight shifting code
        // This will try to create some feeling based on the front tire loads changing to hopefully 'feel' the road more
        // It watches for changes in the left to right split of force
        // 
        // If the front left tire is doing 45% of the work and right front is doing 55%
        // And then they change and the left is doing 40% and the right is 60%
        // You will feel a bump
        // This adds detail to camber and surface changes

    if (speed_mph > 1.0) {
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

            signedMagnitude += static_cast<int>(weightTransferForce * masterForceScale);
            lastFrontImbalance = currentImbalance;
        }
    }

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
    if (std::abs(magnitude - lastSentMagnitude) >= 250 ||     // Reduced from 400
        std::abs(signedMagnitude - lastSentSignedMagnitude) >= 500) { // Reduced from 800
        shouldUpdate = true;
    }
    // 2. More sensitive accumulated changes  
    else if (accumulatedMagnitudeChange >= 200 ||             // Reduced from 300
        accumulatedSignChange >= 400) {                   // Reduced from 600
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
    else if (framesSinceLastUpdate >= 8) {                    // Reduced from 12 (~15Hz)
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



    g_currentFFBForce = signedMagnitude;


    DICONSTANTFORCE cf = { signedMagnitude };  // Use signed magnitude
    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwGain = 10000;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.cAxes = 1;
    DWORD axes[1] = { DIJOFS_X };
    LONG dir[1] = { 0 };  // ← Always zero direction now, this broke Moza wheels
    eff.rgdwAxes = axes;
    eff.rglDirection = dir;
    eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;

    // Only set magnitude params, skip direction
    HRESULT hr = constantForceEffect->SetParameters(&eff, DIEP_TYPESPECIFICPARAMS);  // ← Removed | DIEP_DIRECTION

    if (FAILED(hr)) {
        std::wcerr << L"Constant force SetParameters failed: 0x" << std::hex << hr << std::endl;
    }
}