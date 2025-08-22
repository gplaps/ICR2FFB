#pragma once

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
