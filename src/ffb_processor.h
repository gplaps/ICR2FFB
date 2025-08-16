#pragma once

#include "direct_input.h"
#include "constant_force.h"
#include "damper_effect.h"
#include "ffb_config.h"
#include "ffb_output.h"
#include "lateral_load.h"
#include "spring_effect.h"
#include "telemetry_reader.h"
#include "telemetry_display.h"

struct FFBProcessor {
    FFBProcessor(const FFBConfig& config);
    bool Valid();
    void Update();
    const TelemetryDisplay::TelemetryDisplayData& DisplayData() const;

private:
    void UpdateDisplayData();

    //Get some data from RawTelemetry -> not 100% sure what this does
    RawTelemetry current{};
    RawTelemetry previous{};
    bool firstReading = true;

    int noMovementFrames = 0;
    const int movementThreshold = 3;  // number of frames to consider "stopped"
    bool effectPaused = false;

    //Added for feedback skipping if stopped
    RawTelemetry previousPos{};
    bool firstPos = true;

    int lastConstantForceMagnitude = 0; // does not seem right to have a variable for this
    CalculatedSlip slip{};
    CalculatedVehicleDynamics vehicleDynamics{};
    CalculatedLateralLoad load{};

    ConstantForceEffect constantForceEffect;
    DamperEffect damperEffect;
    SpringEffect springEffect;

    TelemetryReader telemetryReader;
    FFBOutput ffbOutput;
    TelemetryDisplay::TelemetryDisplayData displayData;
    bool mInitialized = false;
};
