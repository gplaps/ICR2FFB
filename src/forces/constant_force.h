#pragma once
#include "project_dependencies.h"

#include "calculations/lateral_load.h"
#include "calculations/slip_angle.h"
#include "calculations/vehicle_dynamics.h"
#include "telemetry_reader.h"

struct ConstantForceEffectResult
{
    ConstantForceEffectResult() = default;
    ConstantForceEffectResult(int magnitude, bool isPaused) :
        magnitude10000(magnitude),
        paused(isPaused) {}
    int  magnitude10000 = 0; // ideally redo the calculation in [0..1] scale (floating point) and let the directInput part do the scaling
    bool paused         = false;
};

struct ConstantForceEffect
{
    ConstantForceEffectResult Calculate(
        const RawTelemetry&              current,
        const CalculatedLateralLoad&     load,
        const CalculatedSlip&            slip,
        const CalculatedVehicleDynamics& vehicleDynamics,
        bool                             enableWeightForce,
        bool                             enableRateLimit,
        double                           masterForceScale,
        double                           deadzoneForcePercentage,
        double                           constantForceScale,
        double                           weightForceScale,
        bool                             invert);
};
