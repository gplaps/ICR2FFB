#pragma once
#include <atomic>

extern std::atomic<bool> shouldExit;
// To be used in reporting - move to TelemetryDisplay
extern int g_currentFFBForce;
