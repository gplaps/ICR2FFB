#include "ffb_processor.h"
#include "constant_force.h"
#include "damper_effect.h"
#include "lateral_load.h"
#include "log.h"
#include "spring_effect.h"
#include "telemetry_display.h"
#include "vehicle_dynamics.h"

FFBProcessor::FFBProcessor(const FFBConfig& config) : telemetryReader(TelemetryReader(config)), ffbOutput(config) {
    if(!telemetryReader.Valid())
    {
        LogMessage(L"[ERROR] TelemetryReader failed to initialize.");
        return;
    }
    if(!ffbOutput.Valid())
        return;

    mInitialized = true;
}

bool FFBProcessor::Valid() { return mInitialized; }

void FFBProcessor::Update() {
    // Check to see if Telemetry is coming in, but if not then wait for it!
    if (!telemetryReader.Update())
        return;

    current = telemetryReader.Data();

    if (firstPos) { previousPos = current; firstPos = false; }

    ffbOutput.Start();

    ffbOutput.Poll();

    // TODO: restructure to be able to simply call ffbOutput.update(constant,damper,spring) and the FFBDevice implements the routing of effect specific processing without any "vehicle dynamics" / "application" logic
     
    // Update Effects
    if (ffbOutput.device.damperEffect && ffbOutput.enableDamperEffect)
    {
        double damperStrength = LowSpeedDamperStrength(current.speed_mph);
        UpdateDamperEffect(damperStrength, ffbOutput.device.damperEffect, ffbOutput.masterForceScale, ffbOutput.damperForceScale);
    }
    
    if (ffbOutput.device.springEffect && ffbOutput.enableSpringEffect)
        UpdateSpringEffect(ffbOutput.device.springEffect, ffbOutput.masterForceScale);

    ffbOutput.Update();

    if (firstReading) {
        previous = current;
        firstReading = false;
    }
    else {
        // Do Force calculations based on raw data
        // Right now its "Slip", "Lateral Load" and "Vehicle Dynamics"
        CalculatedSlip slip{};
        CalculateSlipAngle(current, previous, slip);

        CalculatedVehicleDynamics vehicleDynamics{};
        bool vehicleDynamicsValid = CalculateVehicleDynamics(current, previous, vehicleDynamics);

        CalculatedLateralLoad load{};
        if (CalculateLateralLoad(current, previous, slip, load)) {
            
            // seperation of concern not applied correctly yet ... what is processing, what is "send effect" ... accessing members of members of data is a clear indication that some better structuring needs to take place here!

            // Start constant force once telemetry is valid 
            if (ffbOutput.enableConstantForce && ffbOutput.device.constantForceEffect) {
                if (!ffbOutput.device.constantStarted) {
                    ffbOutput.device.constantForceEffect->Start(1, 0);
                    ffbOutput.device.constantStarted = true;
                    LogMessage(L"[INFO] Constant force started");
                }

                //This is what will add the "Constant Force" effect if all the calculations work. 
                // Probably could smooth all this out
                ApplyConstantForceEffect(current, load, slip, 
                    vehicleDynamics, current.speed_mph, current.steering_deg, ffbOutput.device.constantForceEffect, ffbOutput.enableWeightForce, ffbOutput.enableRateLimit, 
                    ffbOutput.masterForceScale, ffbOutput.deadzoneForceScale,
                    ffbOutput.constantForceScale, ffbOutput.weightForceScale, ffbOutput.invert);
                previousPos = current;

            }

            /*
            // Auto-pause force if not moving
            // I think this is broken or I could detect pause in a better way
            // Maybe DLONG not moving?
            bool isStationary = std::abs(current.dlong - previous.dlong) < 0.01;
            if (isStationary) {
                noMovementFrames++;
                if (noMovementFrames >= movementThreshold && !effectPaused) {
                    //add damper and spring?
                    constantForceEffect->Stop();
                    effectPaused = true;
                    LogMessage(L"[INFO] FFB paused due to no movement");
                }
            }
            else {
                noMovementFrames = 0;
                //  if (effectPaused) {
                    //add damper and spring?
                //      constantForceEffect->Start(1, 0);
                //      effectPaused = false;
                //      std::wcout << L"FFB resumed\n";
                //  }
                // Added Alpha v0.6 to prevent issues with Moza?
                if (effectPaused) {
                    HRESULT startHr = constantForceEffect->Start(1, 0);
                    if (FAILED(startHr)) {
                        LogMessage(L"[ERROR] Failed to restart constant force: 0x" + std::to_wstring(startHr));
                    }
                    else {
                        effectPaused = false;
                        LogMessage(L"[INFO] FFB resumed");
                    }
                }   
            }
            */

            // Update telemetry for display
            {
                std::lock_guard<std::mutex> lock(displayMutex);
                displayData.raw = current;
                displayData.slip = slip;
                // NEW: Vehicle dynamics data (only update if calculation was successful)
                if (vehicleDynamicsValid) {
                    displayData.vehicleDynamics = vehicleDynamics;
                }
                displayData.masterForceValue = ffbOutput.masterForceValue;
            }
        }
    }
}

const TelemetryDisplay::TelemetryDisplayData& FFBProcessor::DisplayData() const {
    return displayData;
}
