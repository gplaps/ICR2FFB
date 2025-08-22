#pragma once

#include "lateral_load.h"
#include "rate_limiter.h"
#include "slip_angle.h"
#include "telemetry_reader.h"
#include "vehicle_dynamics.h"

#include <deque>

struct ConstantForceEffectResult
{
    ConstantForceEffectResult() :
        magnitude10000(),
        paused(true) {}
    ConstantForceEffectResult(int magnitude, bool isPaused) :
        magnitude10000(magnitude),
        paused(isPaused) {}
    int  magnitude10000; // ideally redo the calculation in [0..1] scale (floating point) and let the directInput part do the scaling
    bool paused;
};

struct ConstantForceEffect
{
public:
    ConstantForceEffect() :
        magnitudeHistory() {}

    double                    ApplyDeadzone(double physicsForce, double deadzoneForceScale);
    int                       SmoothSpikes(int signedMagnitude);
    ConstantForceEffectResult Calculate(
        const RawTelemetry&              current,
        const CalculatedLateralLoad&     load,
        const CalculatedSlip&            slip,
        const CalculatedVehicleDynamics& vehicleDynamics,
        bool                             enableRateLimit,
        double                           deadzoneForceScale,
        double                           brakingForceScale,
        double                           weightForceScale);

    RateLimiter     rateLimiter;
    std::deque<int> magnitudeHistory;
};
