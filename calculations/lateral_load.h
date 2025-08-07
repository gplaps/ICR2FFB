#pragma once
#include "telemetry_reader.h"
#include "slip_angle.h"
#include <string>

// Include logging
void LogMessage(const std::wstring& msg);

struct CalculatedLateralLoad {
    double steeringDeg = 0.0;
    double speedMph = 0.0;
    int forceMagnitude = 0;
    int directionVal = 0;
    double lateralG = 0.0;
    double steeringRaw = 0.0;
    double dlong = 0.0;
    double dlat = 0.0; 
};

bool CalculateLateralLoad(const RawTelemetry& current, RawTelemetry& previous, bool& firstReading,
    const CalculatedSlip& slip, CalculatedLateralLoad& out);