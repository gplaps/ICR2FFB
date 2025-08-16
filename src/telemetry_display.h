#pragma once

#include "slip_angle.h"
#include "telemetry_reader.h"
#include "vehicle_dynamics.h"
#include <mutex>

struct TelemetryDisplay {
    // === Shared Telemetry Display Data ===
    struct TelemetryDisplayData {
        RawTelemetry raw;
        CalculatedSlip slip; // Legacy calculated data
        CalculatedVehicleDynamics vehicleDynamics;
        double masterForceValue; // or create another struct containing this if more data is of interest for display
        int constantForceMagnitude;
    };

    TelemetryDisplayData displayData;
    
    // New display
    void DisplayTelemetry(const FFBConfig& config);
    void Update(const FFBConfig& config, const TelemetryDisplayData& tdd);
};

extern std::mutex displayMutex;
