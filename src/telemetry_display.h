#pragma once

#include "constant_force.h"
#include "slip_angle.h"
#include "telemetry_reader.h"
#include "vehicle_dynamics.h"

struct TelemetryDisplay
{
    // === Shared Telemetry Display Data ===
    struct TelemetryDisplayData
    {
        TelemetryDisplayData() :
            raw(),
            slip(),
            vehicleDynamics(),
            masterForceValue(0.0) {}
        RawTelemetry              raw;
        CalculatedSlip            slip; // Legacy calculated data
        CalculatedVehicleDynamics vehicleDynamics;
        double                    masterForceValue; // or create another struct containing this if more data is of interest for display
        ConstantForceEffectResult constantForce;
    };

    TelemetryDisplayData displayData;

    // New display
    void DisplayTelemetry(const FFBConfig& config) const;
    void Update(const FFBConfig& config, const TelemetryDisplayData& displayDataIn);

    static DECLARE_MUTEX(mutex);
};
