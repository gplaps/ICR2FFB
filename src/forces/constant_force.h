#pragma once
#include "calculations/slip_angle.h"
#include "calculations/lateral_load.h"
#include "calculations/vehicle_dynamics.h"
#include "project_dependencies.h"
#include <string>

void ApplyConstantForceEffect(
    const RawTelemetry& current,
    const CalculatedLateralLoad& load,
    const CalculatedSlip& slip,
    const CalculatedVehicleDynamics& vehicleDynamics,
    double speed_mph,
    double steering_deg,
    IDirectInputEffect* effect,
    bool enableWeightForce,
    bool enableRateLimit,
    double masterForceScale,
    double deadzoneForceScale,
    double constantForceScale,
    double weightForceScale,
    bool invert
    );
