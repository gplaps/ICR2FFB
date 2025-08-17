#pragma once
#include "telemetry_reader.h"

struct CalculatedVehicleDynamics
{
    double lateralG;
    int    directionVal;
    double yaw;
    double slip;
    double forceMagnitude;
    double speedMph;
    double steeringDeg;

    // Individual tire forces
    double force_lf;
    double force_rf;
    double force_lr;
    double force_rr;

    // Aggregate forces / Additional calculated values
    double frontLateralForce;
    double rearLateralForce;
    double totalLateralForce;
    double yawMoment;

    double frontLeftForce_N;
    double frontRightForce_N;

    bool Calculate(const RawTelemetry& current, RawTelemetry& /*previous*/);
};
