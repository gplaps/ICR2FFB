#include "ffb_processor.h"

#include "lateral_load.h"
#include "log.h"
#include "telemetry_display.h"
#include "vehicle_dynamics.h"

FFBProcessor::FFBProcessor(const FFBConfig& config) :
    current(),
    previous(),
    hasFirstReading(false),
    noMovementFrames(0),
    movementThreshold(3),
    effectPaused(false),
    previousPos(),
    hasFirstPos(false),
    slip(),
    vehicleDynamics(),
    load(),
    telemetryReader(TelemetryReader(config)),
    ffbOutput(config),
    displayData()
{
    if (!telemetryReader.Valid())
    {
        LogMessage(L"[ERROR] TelemetryReader failed to initialize.");
        return;
    }
    if (!ffbOutput.Valid())
        return;

    mInitialized = true;
}

bool FFBProcessor::Valid() const { return mInitialized; }

void FFBProcessor::Update()
{
    // Check to see if Telemetry is coming in, but if not then wait for it!
    if (!telemetryReader.Update())
        return;

    current = telemetryReader.Data();

    if (!hasFirstPos)
    {
        previousPos = current;
        hasFirstPos = true;
    }

    ffbOutput.Start();

    ffbOutput.Poll();

    // TODO: restructure to be able to simply call ffbOutput.update(constant,damper,spring) with final effect scales, the FFBDevice implements the directInput API / effect specific processing without any "vehicle dynamics" / "application" logic. structuring of FFBOutput and FFBProcessor is not good yet

    // Update Effects
    ffbOutput.UpdateDamper(current.speed_mph);
    ffbOutput.UpdateSpring();

    ffbOutput.Update();

    if (!hasFirstReading)
    {
        previous        = current;
        hasFirstReading = true;
    }
    else
    {
        if (ProcessTelemetryInput())
        {
            // seperation of concern not applied fully yet ... what is processing, what is "send effect" ... accessing members of members of data is a clear indication that some better structuring needs to take place! don't "reaching through" modules

            if (ffbOutput.enableConstantForce)
            {
                //This is what will add the "Constant Force" effect if all the calculations work.
                // Probably could smooth all this out
                constantForceCalculation = constantForceEffect.Calculate(
                    current, load, slip, vehicleDynamics,                   // inputs
                    ffbOutput.enableWeightForce, ffbOutput.enableRateLimit, // settings - thats where it might need restructuring of the implementation, probably all the scales should be kept in FFBOutput and not be added in this function. likely make the Result struct contain seperate channels and do the multiply with scales in FFBOutput depending on the enable flags
                    ffbOutput.masterForceScale, ffbOutput.deadzoneForceScale,
                    ffbOutput.constantForceScale, ffbOutput.weightForceScale, ffbOutput.invert);
                previousPos = current;

                ffbOutput.UpdateConstantForce(constantForceCalculation);
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

bool FFBProcessor::ProcessTelemetryInput()
{
    // Do Force calculations based on raw data
    // Right now its "Slip", "Lateral Load" and "Vehicle Dynamics"
    slip = CalculatedSlip();
    slip.Calculate(current, previous);

    vehicleDynamics = CalculatedVehicleDynamics();
    vehicleDynamics.Calculate(current, previous);

    load = CalculatedLateralLoad();
    return load.Calculate(current, previous, slip);
}

void FFBProcessor::UpdateDisplayData()
{
    LOCK_MUTEX(displayMutex);

    // Update telemetry for display
    displayData.raw              = current;
    displayData.slip             = slip;
    displayData.vehicleDynamics  = vehicleDynamics;

    displayData.masterForceValue = ffbOutput.masterForceValue;
    displayData.constantForce    = constantForceCalculation;

    UNLOCK_MUTEX(displayMutex);
}

const TelemetryDisplay::TelemetryDisplayData& FFBProcessor::DisplayData() const
{
    return displayData;
}
