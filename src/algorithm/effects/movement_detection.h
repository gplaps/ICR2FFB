#pragma once
#include "telemetry_reader.h"

// Beta 0.5
// This is a bunch of logic to pause/unpause or prevent forces when the game isn't running
// I think this could probably be a lot simpler so maybe up for a redo
// It works right now though, although it feels a bit delayed

struct MovementDetector
{
    // derived from telemetry
    // unfortunately a mix of both simulation state (paused, running, in replay, in menu) and vehicle movement state
    enum MovementState
#if defined(IS_CPP11_COMPLIANT)
        : unsigned char
#endif
    {
        MS_UNKNOWN,
        MS_IDLE,
        MS_DRIVING
    };

    MovementDetector();
    MovementState Calculate(const RawTelemetry& current, const RawTelemetry& previous /* unused but consider removing some state in this struct and derive from the RawTelemetry delta in every loop */);

    double       lastDlong;
    int          noMovementFrames;
    bool         isPaused;
    bool         pauseForceSet;
    bool         isFirstReading;
    const int    movementThreshold       = 10;    // Frames to consider "paused"
    const double movementThreshold_value = 0.001; // Very small movement threshold

    // deprecated variables - likely delete
    // bool hasEverMoved = false;
    // bool      effectPaused;
};
