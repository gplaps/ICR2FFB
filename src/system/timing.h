#pragma once

#include "ffb_config.h"

struct ThreadTimer
{
    explicit ThreadTimer(double intervalMs, bool log = false) :
        nextTime(),
        interval(intervalMs),
        report(log) {}

    double nextTime;
    double interval;
    bool   report;

    bool ready();
    void schedule() const;
};

struct Timing
{
    explicit Timing(const FFBConfig& config);
    ~Timing();

    // Timing buffers
    ThreadTimer ffb;
    ThreadTimer telemetry;
    ThreadTimer render;
};

double TimeSinceStartInMs();
