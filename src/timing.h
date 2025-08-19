#pragma once
#include "project_dependencies.h"

#define PRINT_INTERVAL 66.68     // log timing ~15fps
#define TELEMETRY_INTERVAL 16.67  // ~60 FPS telemetry
#define FFB_INTERVAL 16.67        // ~60 FPS FFB

struct ThreadTimer {
    explicit ThreadTimer(double intervalMs, bool precise = true) : 
    nextTime(),
    interval(intervalMs), keepGoodTiming(precise) {}

    double nextTime;
    double interval;
    bool keepGoodTiming;

    bool canStart();
    void finished() const;
};

struct Timing {
    Timing();

    // Timing buffers
    ThreadTimer ffb;
    ThreadTimer telemetry;
    ThreadTimer render;
};
