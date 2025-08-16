#pragma once
#include "calculations/slip_angle.h"
#include "calculations/lateral_load.h"
#include "calculations/vehicle_dynamics.h"
#include "ffb_device.h"
#include "project_dependencies.h"
#include <string>

struct ConstantForceEffect {
    int Apply(
        const RawTelemetry& current,
        const CalculatedLateralLoad& load,
        const CalculatedSlip& slip,
        const CalculatedVehicleDynamics& vehicleDynamics,
        double speed_mph,
        double steering_deg,
        FFBDevice& device,
        bool enableWeightForce,
        bool enableRateLimit,
        double masterForceScale,
        double deadzoneForceScale,
        double constantForceScale,
        double weightForceScale,
        bool invert
        );
};
