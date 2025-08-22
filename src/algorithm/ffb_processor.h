#pragma once

#include "constant_force.h"
#include "damper_effect.h"
#include "ffb_config.h"
#include "ffb_output.h"
#include "lateral_load.h"
#include "movement_detection.h"
#include "slip_angle.h"
#include "spring_effect.h"
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

    void Init(const FFBConfig& config);

    // ====================================================================
    // === Step1 - raw telemetry to "cleaned up" / "engineering units" data
    // ====================================================================

    RawTelemetry current;
    RawTelemetry previous;
    bool         hasFirstReading;

    MovementDetector movementDetector;

    bool enableRateLimit;

    // ================================================
    // === Step 2 - vehicle dynamics to FFB effects ===
    // ================================================

    // FFB Algo Inputs - telemetry derived data
    CalculatedSlip            slip;
    CalculatedVehicleDynamics vehicleDynamics;
    CalculatedLateralLoad     load;

    // FFB Algo Outputs / calculations
    ConstantForceEffectResult constantForceCalculation;
    ConstantForceEffect       constantForceEffect;
    DamperEffect              damperEffect;
    SpringEffect              springEffect;

    // settings
    double deadzoneForceScale;
    double brakingForceScale;
    double weightForceScale;

    // IO
    TelemetryReader                        telemetryReader;
    FFBOutput                              ffbOutput;
    TelemetryDisplay::TelemetryDisplayData displayData;

    bool mInitialized;
};
