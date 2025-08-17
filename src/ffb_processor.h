#pragma once

#include "constant_force.h"
#include "direct_input.h"
#include "ffb_config.h"
#include "ffb_output.h"
#include "lateral_load.h"
#include "slip_angle.h"
#include "telemetry_display.h"
#include "telemetry_reader.h"
#include "vehicle_dynamics.h"

struct FFBProcessor
{
    explicit FFBProcessor(const FFBConfig& config);
    bool                                          Valid() const;
    void                                          Update();
    const TelemetryDisplay::TelemetryDisplayData& DisplayData() const;

private:
    void UpdateDisplayData();
    bool ProcessTelemetryInput();

    //Get some data from RawTelemetry -> not 100% sure what this does
    RawTelemetry current;
    RawTelemetry previous;
    bool         hasFirstReading   ;

    int       noMovementFrames  ;
    const int movementThreshold ; // number of frames to consider "stopped" - defaults to 3
    bool      effectPaused      ;

    //Added for feedback skipping if stopped
    RawTelemetry previousPos;
    bool         hasFirstPos;

    ConstantForceEffectResult constantForceCalculation;

    CalculatedSlip            slip;
    CalculatedVehicleDynamics vehicleDynamics;
    CalculatedLateralLoad     load;

    ConstantForceEffect constantForceEffect;

    TelemetryReader                        telemetryReader;
    FFBOutput                              ffbOutput;
    TelemetryDisplay::TelemetryDisplayData displayData;
    bool                                   mInitialized;
};
