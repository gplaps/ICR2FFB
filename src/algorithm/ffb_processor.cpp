#include "ffb_processor.h"

#include "constant_force.h"
#include "constants.h"
#include "lateral_load.h"
#include "log.h"
#include "math_utilities.h"
#include "movement_detection.h"
#include "telemetry_display.h"
#include "vehicle_dynamics.h"

FFBProcessor::FFBProcessor(const FFBConfig& config) :
    current(),
    previous(),
    hasFirstReading(false),
    movementDetector(),
    enableRateLimit(false),
    slip(),
    vehicleDynamics(),
    load(),
    constantForceEffect(),
    damperEffect(config),
    springEffect(),
    deadzoneForceScale(),
    brakingForceScale(),
    weightForceScale(),
    telemetryReader(config),
    ffbOutput(config),
    displayData(),
    mInitialized(false)
{
    if (!telemetryReader.Valid())
    {
        LogMessage(L"[ERROR] TelemetryReader failed to initialize.");
        return;
    }
    if (!ffbOutput.Valid())
    {
        LogMessage(L"[ERROR] FFBOut failed to initialize.");
        return;
    }

    mInitialized = true;
}

void FFBProcessor::Init(const FFBConfig& config)
{
    // Parse FFB effect toggles from config <- should all ffb types be enabled? Allows user to select if they dont like damper for instance
    // Would be nice to add a % per effect in the future
    enableRateLimit              = config.GetBool(L"effects", L"limit");
    const bool enableWeightForce = config.GetBool(L"effects", L"weight");

    deadzoneForceScale           = saturate(config.GetDouble(L"effects", L"deadzone") / 100.0);
    weightForceScale             = enableWeightForce ? saturate(config.GetDouble(L"effects", L"weight scale") / 100.0) : 0.0;
    brakingForceScale            = std::clamp(config.GetDouble(L"effects", L"braking scale") / 100.0, 0.0, 10.0 /* upper limit subject to change - this restricts user*/);
}

bool FFBProcessor::Valid() const { return mInitialized; }

void FFBProcessor::Update(double deltaTimeMs)
{
    // Check to see if Telemetry is coming in, but if not then wait for it!
    if (!telemetryReader.Update())
    {
        return;
    }

    current = telemetryReader.Data();

    ffbOutput.Start();
    ffbOutput.Poll();

    // The goal should be to simply call ffbOutput.update(constant,damper,spring) with final effect scale and keep application logic / force calculation in this class, FFBOutput just scales and toggles the effect strengths and the FFBDevice implements the directInput API translation.
    // Effect calculations are "orchestrated" in FFBProcessor ... with how many submodules seem right

    // Update Effects results
    double                          damperStrength           = 0.0;
    double                          springStrength           = 0.0;
    MovementDetector::MovementState movementState            = MovementDetector::MS_UNKNOWN;
    ConstantForceEffectResult       constantForceCalculation = ConstantForceEffectResult();

    if (!hasFirstReading)
    {
        hasFirstReading = true;
    }
    else
    {
        if (ProcessTelemetryInput())
        {
            // Update Effects
            damperStrength = damperEffect.Calculate(current.speed_mph);
            springStrength = springEffect.Calculate(); // constant, nothing "dynamic", see comments in SpringEffect

            movementState  = movementDetector.Calculate(current, previous, deltaTimeMs);
            if (movementState == MovementDetector::MS_DRIVING)
            {
                //This is what will add the "Constant Force" effect if all the calculations work.
                // Probably could smooth all this out
                constantForceCalculation = constantForceEffect.Calculate(
                    current, load, slip, vehicleDynamics, // inputs
                    enableRateLimit,                      // settings
                    deadzoneForceScale, brakingForceScale, weightForceScale);
            }
        }
    }
    ffbOutput.Update(constantForceCalculation.magnitude, damperStrength, springStrength, constantForceCalculation.paused);
    UpdateDisplayData(constantForceCalculation);
    previous = current;
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

void FFBProcessor::UpdateDisplayData(const ConstantForceEffectResult& constantResult)
{
    LOCK_MUTEX(TelemetryDisplay::mutex);

    // Update telemetry for display
    displayData.raw              = current;
    displayData.slip             = slip;
    displayData.vehicleDynamics  = vehicleDynamics;

    displayData.masterForceScale = ffbOutput.masterForceScale;
    displayData.constantForce    = constantResult;

    UNLOCK_MUTEX(TelemetryDisplay::mutex);
}

const TelemetryDisplay::TelemetryDisplayData& FFBProcessor::DisplayData() const
{
    return displayData;
}
