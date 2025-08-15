#pragma once
#include "telemetry_reader.h"
#include <string>

struct CalculatedSlip {
    double slipAngle;
    double absSlipDeg;
    double forceMagnitude;
    int directionVal;
    bool suppressFrame;
    double slipNorm_lf, slipNorm_rf, slipNorm_lr, slipNorm_rr;
    double slipMag_lf, slipMag_rf, slipMag_lr, slipMag_rr;
};

bool CalculateSlipAngle(const RawTelemetry& current, RawTelemetry& previous, bool& firstReading, CalculatedSlip& out);
