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
    double forceLong_lf;
    double forceLong_rf;
    double forceLong_lr;
    double forceLong_rr;

    // Aggregate forces / Additional calculated values
    double frontLateralForce;
    double rearLateralForce;
    double totalLateralForce;
    double yawMoment;
    // double frontLongitudinalForce;
    // double rearLongitudinalForce;
    // double totalLongitudinalForce;

    double frontLeftForce_N;
    double frontRightForce_N;
    double frontLeftLong_N;
    double frontRightLong_N;

    bool Calculate(const RawTelemetry& current, RawTelemetry& /*previous*/);
};
