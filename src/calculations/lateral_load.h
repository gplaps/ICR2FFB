#pragma once
#include "slip_angle.h"
#include "telemetry_reader.h"

struct CalculatedLateralLoad
{
    double steeringDeg;
    double speedMph;
    double forceMagnitude;
    int    directionVal;
    double lateralG;
    double steeringRaw;
    double dlong;
    double dlat;

    bool Calculate(const RawTelemetry& current, RawTelemetry& /*previous*/, const CalculatedSlip& slip);
};
