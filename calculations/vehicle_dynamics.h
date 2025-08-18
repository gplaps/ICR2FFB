#pragma once
#include "telemetry_reader.h"
#include <string>

// Include logging
void LogMessage(const std::wstring& msg);

struct CalculatedVehicleDynamics {
    double lateralG = 0.0;
    int directionVal = 0;
    double yaw = 0.0;
    double slip = 0.0;
    int forceMagnitude = 0;
    double speedMph = 0.0;
    double steeringDeg = 0.0;

    // Individual tire forces for debugging
    double force_lf = 0.0;
    double force_rf = 0.0;
    double force_lr = 0.0;
    double force_rr = 0.0;
    double forceLong_lf = 0.0;
    double forceLong_rf = 0.0;
    double forceLong_lr = 0.0;
    double forceLong_rr = 0.0;

    // Additional calculated values
    double frontLateralForce = 0.0;
    double rearLateralForce = 0.0;
    double totalLateralForce = 0.0;
   // double frontLongitudinalForce = 0.0;
   // double rearLongitudinalForce = 0.0;
   // double totalLongitudinalForce = 0.0;
    double yawMoment = 0.0;

    double frontLeftForce_N;
    double frontRightForce_N;
    double frontLeftLong_N;
    double frontRightLong_N;
};

bool CalculateVehicleDynamics(const RawTelemetry& current, RawTelemetry& previous, bool& firstReading, CalculatedVehicleDynamics& out);