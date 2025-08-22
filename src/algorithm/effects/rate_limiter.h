#pragma once

#include "project_dependencies.h"

struct RateLimiter
{
    RateLimiter();

    int Calculate(int signedMagnitude, double force);
    // Direction calculation and smoothing for rate limiting
    LONG lastDirection;

    // Direction smoothing - this prevents rapid direction changes
    const double directionSmoothingFactor;

    // Rate limiting with direction smoothing
    int    lastSentMagnitude;
    int    lastSentSignedMagnitude;
    LONG   lastSentDirection; // Track smoothed direction
    int    lastProcessedMagnitude;
    int    framesSinceLastUpdate;
    double accumulatedMagnitudeChange;
    double accumulatedDirectionChange; // Track direction changes
};
