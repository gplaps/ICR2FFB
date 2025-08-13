#pragma once
#include "telemetry_reader.h"
#include <string>

// Include logging
void LogMessage(const std::wstring &msg);

struct CalculatedSlip
{
    double slipAngle = 0.0;
    double absSlipDeg = 0.0;
    int forceMagnitude = 0;
    int directionVal = 0;
    bool suppressFrame = false;
    double slipNorm_lf, slipNorm_rf, slipNorm_lr, slipNorm_rr;
    double slipMag_lf, slipMag_rf, slipMag_lr, slipMag_rr;
};

bool CalculateSlipAngle(const RawTelemetry &current, RawTelemetry &previous, bool &firstReading, CalculatedSlip &out);
