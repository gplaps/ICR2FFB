#pragma once

#include "constant_force.h"
#include "slip_angle.h"
#include "telemetry_reader.h"
#include "vehicle_dynamics.h"

#if defined(HAS_STL_THREAD_MUTEX)
#include <mutex>
extern std::mutex displayMutex;
#else
extern HANDLE displayMutex;
#endif

struct TelemetryDisplay
{
    // === Shared Telemetry Display Data ===
    struct TelemetryDisplayData
    {
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
};
