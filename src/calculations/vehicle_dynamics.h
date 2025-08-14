#pragma once
#include "telemetry_reader.h"
#include <string>

// Include logging
void LogMessage(const std::wstring& msg);

struct CalculatedVehicleDynamics {
    double lateralG;
    int directionVal;
    double yaw;
    double slip;
    double forceMagnitude;
    double speedMph;
    double steeringDeg;

    // Individual tire forces for debugging
    double force_lf;
    double force_rf;
    double force_lr;
    double force_rr;

    // Additional calculated values
    double frontLateralForce;
    double rearLateralForce;
    double totalLateralForce;
    double yawMoment;

    double frontLeftForce_N;
    double frontRightForce_N;
};

bool CalculateVehicleDynamics(const RawTelemetry& current, RawTelemetry& previous, bool& firstReading, CalculatedVehicleDynamics& out);
