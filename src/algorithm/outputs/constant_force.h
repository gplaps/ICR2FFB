#pragma once

#include "lateral_load.h"
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

struct RateLimiter
{
    RateLimiter();

    int Calculate(int signedMagnitude, double force);
    // Direction calculation and smoothing for rate limiting
    LONG lastDirection = 0;

    // Direction smoothing - this prevents rapid direction changes
    const double directionSmoothingFactor = 0.3;

    // Rate limiting with direction smoothing
    int    lastSentMagnitude;
    int    lastSentSignedMagnitude;
    LONG   lastSentDirection; // Track smoothed direction
    int    lastProcessedMagnitude;
    int    framesSinceLastUpdate;
    double accumulatedMagnitudeChange;
    double accumulatedDirectionChange; // Track direction changes
};

struct ConstantForceEffect
{
public:
    ConstantForceEffect() :
        magnitudeHistory() {}

    double                    ApplyDeadzone(double physicsForce, double deadzoneForceScale);
    ConstantForceEffectResult Calculate(
        const RawTelemetry&              current,
        const CalculatedLateralLoad&     load,
        const CalculatedSlip&            slip,
        const CalculatedVehicleDynamics& vehicleDynamics,
        bool                             enableRateLimit,
        double                           deadzoneForcePercentage,
        double                           brakingForceScale,
        double                           weightForceScale);

    RateLimiter     rateLimiter;
    std::deque<int> magnitudeHistory;
};
