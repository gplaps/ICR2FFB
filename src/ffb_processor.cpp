#include "ffb_processor.h"
#include "lateral_load.h"
#include "log.h"
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

    // TODO: restructure to be able to simply call ffbOutput.update(constant,damper,spring) with final effect scales, the FFBDevice implements the directInput API / effect specific processing without any "vehicle dynamics" / "application" logic. structuring of FFBOutput and FFBProcessor is not good yet
     
    // Update Effects
    ffbOutput.UpdateDamper(current.speed_mph);
    ffbOutput.UpdateSpring();
    
    ffbOutput.Update();

    if (firstReading) {
        previous = current;
        firstReading = false;
    }
    else {
        // Do Force calculations based on raw data
        // Right now its "Slip", "Lateral Load" and "Vehicle Dynamics"
        slip = {};
        slip.Calculate(current, previous);

        vehicleDynamics = {};
        vehicleDynamics.Calculate(current, previous);

        load = {};
        if (load.Calculate(current, previous, slip)) {
            // seperation of concern not applied fully yet ... what is processing, what is "send effect" ... accessing members of members of data is a clear indication that some better structuring needs to take place! don't "reaching through" modules

            // Start constant force once telemetry is valid 
            if (ffbOutput.enableConstantForce) {
                ffbOutput.device.StartConstant();

                //This is what will add the "Constant Force" effect if all the calculations work. 
                // Probably could smooth all this out
                 lastConstantForceMagnitude = constantForceEffect.Apply(current, load, slip, 
                    vehicleDynamics, current.speed_mph, current.steering_deg, ffbOutput.device, ffbOutput.enableWeightForce, ffbOutput.enableRateLimit, 
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

            UpdateDisplayData();
        }
    }
}

void FFBProcessor::UpdateDisplayData() {
    // Update telemetry for display
    std::lock_guard<std::mutex> lock(displayMutex);
    displayData.raw = current;
    displayData.slip = slip;
    // NEW: Vehicle dynamics data (only update if calculation was successful)
    displayData.vehicleDynamics = vehicleDynamics;

    displayData.masterForceValue = ffbOutput.masterForceValue;
    displayData.constantForceMagnitude = lastConstantForceMagnitude;
}

const TelemetryDisplay::TelemetryDisplayData& FFBProcessor::DisplayData() const {
    return displayData;
}
