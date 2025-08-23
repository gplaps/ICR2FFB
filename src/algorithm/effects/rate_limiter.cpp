#include "rate_limiter.h"

#include "constants.h"
#include "math_utilities.h"

RateLimiter::RateLimiter() :
    lastDirection(0),
    directionSmoothingFactor(0.3),
    lastSentMagnitude(-1),
    lastSentSignedMagnitude(0),
    lastSentDirection(0),
    lastProcessedMagnitude(-1),
    framesSinceLastUpdate(0),
    accumulatedMagnitudeChange(0.0),
    accumulatedDirectionChange(0.0) {}

int RateLimiter::Calculate(int signedMagnitude, double force)
{
    int magnitude = std::abs(signedMagnitude);

    // Direction calculation and smoothing for rate limiting
    const double targetDir = -sign(force) * MAX_FORCE_IN_N;

    // Direction smoothing - this prevents rapid direction changes
    lastDirection = static_cast<LONG>(lerp(targetDir, static_cast<double>(lastDirection), directionSmoothingFactor));

    // Track accumulated changes since last update
    if (lastSentMagnitude != -1)
    {
        accumulatedMagnitudeChange += std::abs(magnitude - lastProcessedMagnitude);
        accumulatedDirectionChange += std::abs(lastDirection - lastSentDirection); // Use smoothed direction
    }

    framesSinceLastUpdate++;
    bool shouldUpdate = false;

    // 1. Large immediate change
    if (std::abs(magnitude - lastSentMagnitude) >= 400 ||
        std::abs(lastDirection - lastSentDirection) >= 2000)
    { // Use smoothed direction
        shouldUpdate = true;
    }
    // 2. Accumulated changes
    else if (accumulatedMagnitudeChange >= 300 ||
             accumulatedDirectionChange >= 1500)
    { // Use direction accumulation
        shouldUpdate = true;
    }
    // 3. Direction sign change (use smoothed direction)
    else if ((lastSentDirection > 0 && lastDirection < 0) ||
             (lastSentDirection < 0 && lastDirection > 0) ||
             (lastSentDirection == 0 && lastDirection != 0) ||
             (lastSentDirection != 0 && lastDirection == 0))
    {
        shouldUpdate = true;
    }
    // 4. Timeout
    else if (framesSinceLastUpdate >= 12)
    {
        shouldUpdate = true;
    }
    // 5. Zero force
    else if (magnitude == 0 && lastSentMagnitude != 0)
    {
        shouldUpdate = true;
    }

    if (!shouldUpdate)
    {
        lastProcessedMagnitude = magnitude;
        return 0; //ConstantForceEffectResult(0, false); // Skip this frame
    }

    // Reset tracking when we send an update
    lastSentMagnitude          = magnitude;
    lastSentSignedMagnitude    = signedMagnitude;
    lastSentDirection          = lastDirection; // Track the smoothed direction
    framesSinceLastUpdate      = 0;
    accumulatedMagnitudeChange = 0.0;
    accumulatedDirectionChange = 0.0;
    lastProcessedMagnitude     = magnitude;

    return signedMagnitude;
}
