#pragma once
#include "telemetry_reader.h"
#include "slip_angle.h"
#include <string>

struct CalculatedLateralLoad {
    double steeringDeg;
    double speedMph;
    double forceMagnitude;
    int directionVal;
    double lateralG;
    double steeringRaw;
    double dlong;
    double dlat; 
};

bool CalculateLateralLoad(const RawTelemetry& current, RawTelemetry& previous, bool& firstReading,
    const CalculatedSlip& slip, CalculatedLateralLoad& out);
