#pragma once

#define PRINT_INTERVAL     66.68 // log timing ~15fps
#define TELEMETRY_INTERVAL 16.67 // ~60 FPS telemetry
#define FFB_INTERVAL       16.67 // ~60 FPS FFB

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
    Timing();

    // Timing buffers
    ThreadTimer ffb;
    ThreadTimer telemetry;
    ThreadTimer render;
};
