#pragma once
#include "telemetry_reader.h"

enum SimState // unused, but can maybe be derived or read directly
{
    ST_UNKNOWN,
    ST_STOPPED,
    ST_PAUSED,
    ST_RUNNING,
    ST_REPLAY
};

// Beta 0.5
// This is a bunch of logic to pause/unpause or prevent forces when the game isn't running
// I think this could probably be a lot simpler so maybe up for a redo
// It works right now though, although it feels a bit delayed

struct MovementDetector
{
    enum MovementState
    { // likly the simulation state variable may be accessible, otherwise the "in menu"/"in driving mode" and "simulation paused", "simulation running"/"simulation replay"
        MS_UNKNOWN,
        MS_IDLE,
        MS_DRIVING
    };

    MovementDetector();
    MovementState Calculate(const RawTelemetry& current);

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
