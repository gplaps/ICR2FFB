#include "lateral_load.h"

#include "constants.h"
#include "math_utilities.h"

#include <cmath>

// Pretend "Lateral G" calculation
// Idea is the force should be a direct opposite to the lateral G so the driver "Feels" the pull of the forces
// You should have to push your wheel against the force

bool CalculatedLateralLoad::Calculate(const RawTelemetry& current, RawTelemetry& /*previous*/, const CalculatedSlip& slip)
{
    // Estimates based on "Slip"
    // Take the forces the tires do and figure out the difference between front and back
    const double avgSlip = (slip.slipNorm_lf + slip.slipNorm_rf) * 0.5;

    // Output variables
    speedMph    = current.speed_mph;
    steeringDeg = current.steering_deg;
    steeringRaw = current.steering_raw;
    // assuming going around Indy at 220mph is 4G, this looks ABOUT right
    lateralG = avgSlip * 16.0; // 0.25 slipNorm ≈ 4G;

    // FFB scaling: assume max useful G is 8. It should make whatever "Max" force is top out at "8" G
    forceMagnitude = saturate(std::abs(lateralG / MAX_USEFUL_G));

    // Direction: force toward inside of corner
    // You want the wheel to push against you
    directionVal = static_cast<int>(-sign(lateralG));

    return true;
}
