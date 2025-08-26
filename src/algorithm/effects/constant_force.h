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
        magnitude(),
        paused(true) {}
    ConstantForceEffectResult(double magnitude01, bool isPaused) :
        magnitude(magnitude01),
        paused(isPaused) {}
    double magnitude;
    bool   paused;
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
