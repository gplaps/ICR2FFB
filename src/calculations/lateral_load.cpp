#include "lateral_load.h"
#include "constants.h"
#include "helpers.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <deque>
#include <numeric>

// Pretend "Lateral G" calculation
// Idea is the force should be a direct opposite to the lateral G so the driver "Feels" the pull of the forces
// You should have to push your wheel against the force

bool CalculateLateralLoad(const RawTelemetry& current, RawTelemetry& /*previous*/,
    const CalculatedSlip& slip, CalculatedLateralLoad& out) {
    // Estimates based on "Slip"
    // Take the forces the tires do and figure out the difference between front and back
    double avgSlip = (slip.slipNorm_lf + slip.slipNorm_rf) * 0.5;

    // assuming going around Indy at 220mph is 4G, this looks ABOUT right
    double lateralG = avgSlip * 16.0;  // 0.25 slipNorm ≈ 4G

    // Output variables
    out.speedMph = current.speed_mph;
    out.steeringDeg = current.steering_deg;
    out.steeringRaw = current.steering_raw;
    out.lateralG = lateralG;

    // FFB scaling: assume max useful G is 8. It should make whatever "Max" force is top out at "8" G
    out.forceMagnitude = std::clamp(std::abs(lateralG / MAX_USEFUL_G), 0.0, 1.0);

    // Direction: force toward inside of corner
    // You want the wheel to push against you
    out.directionVal = -sign(lateralG);

    return true;
}
