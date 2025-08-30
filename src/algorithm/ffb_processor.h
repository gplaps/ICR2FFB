#pragma once

#include "constant_force.h"
#include "damper_effect.h"
#include "ffb_config.h"
#include "ffb_output.h"
#include "game_version.h"
#include "movement_detection.h"
#include "spring_effect.h"
#include "telemetry_display.h"
#include "telemetry_reader.h"
#include "vehicle_dynamics.h"

struct FFBProcessor
{
    explicit FFBProcessor(FFBConfig& config);
    bool                                          Valid() const;
    void                                          Update(double deltaTimeMs);
    const TelemetryDisplay::TelemetryDisplayData& DisplayData() const;

private:
    void UpdateDisplayData(const ConstantForceEffectResult& constantResult);
    bool ProcessTelemetryInput();

    void Init(const FFBConfig& config);

    // ====================================================================
    // === Step1 - raw telemetry to "cleaned up" / "engineering units" data
    // ====================================================================

    RawTelemetry current;
    RawTelemetry previous;
    bool         hasFirstReading;

    // ================================================
    // === Step 2 - vehicle dynamics to FFB effects ===
    // ================================================

    // FFB Algo Inputs - telemetry derived data
    CalculatedVehicleDynamics vehicleDynamics;

    MovementDetector movementDetector;

    // FFB Algo Outputs / calculations
    ConstantForceEffect constantForceEffect;
    DamperEffect        damperEffect;
    SpringEffect        springEffect;

    // settings
    double deadzoneForceScale;
    double brakingForceScale;
    double weightForceScale;
    bool   enableRateLimit;

    // IO
    FFBOutput                              ffbOutput;
    TelemetryReader                        telemetryReader;
    TelemetryDisplay::TelemetryDisplayData displayData;

    Game detectedGame;
    bool mInitialized;
};
