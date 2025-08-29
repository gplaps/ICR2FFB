#include "vehicle_dynamics.h"

#include "constants.h"
#include "math_utilities.h"
#include "string_utilities.h"

#include <cmath>
#if !defined(IS_CPP11_COMPLIANT)
#    include <stdint.h>
#endif

#include <algorithm>

// SAE Convention
// Lateral force
// Negative <---------> Positive

// Left Turn
// Lateral G -> Positive
// Oversteer Slip -> Positive
// Understeer Slip -> Negative


struct GameConstants
{
    GameConstants(double VEHICLE_MASS_IN,
                  double FRONT_TRACK_IN,
                  double REAR_TRACK_IN,
                  double WHEELBASE_IN,
                  double YAW_INERTIA_IN,
                  double CG_FROM_FRONT_IN,
                  double CG_FROM_REAR_IN,
                  double TIRE_FORCE_SCALE_IN,
                  double MAX_GAME_FORCE_UNITS_IN) :
        VEHICLE_MASS(VEHICLE_MASS_IN),
        FRONT_TRACK(FRONT_TRACK_IN),
        REAR_TRACK(REAR_TRACK_IN),
        WHEELBASE(WHEELBASE_IN),
        YAW_INERTIA(YAW_INERTIA_IN),
        CG_FROM_FRONT(CG_FROM_FRONT_IN),
        CG_FROM_REAR(CG_FROM_REAR_IN),
        TIRE_FORCE_SCALE(TIRE_FORCE_SCALE_IN),
        MAX_GAME_FORCE_UNITS(MAX_GAME_FORCE_UNITS_IN) {}

    double VEHICLE_MASS;
    double FRONT_TRACK;
    double REAR_TRACK;
    double WHEELBASE;
    double YAW_INERTIA;
    double CG_FROM_FRONT;
    double CG_FROM_REAR;
    double TIRE_FORCE_SCALE;
    double MAX_GAME_FORCE_UNITS;
};

// Vehicle Constants
// Most of this is estimates/guesses
// May or may not be used

namespace VehicleConstants
{
const double GRAVITY = 9.81; // m/s² (same for all)

static GameConstants IndyCar(
    /*VEHICLE_MASS =                           */ 700.0 + 60.0 + 76.0, // kg (car + fuel + driver)
    /*FRONT_TRACK =                            */ 1.753,               // m
    /*REAR_TRACK =                             */ 1.638,               // m
    /*WHEELBASE =                              */ 3.048,               // m
    /*YAW_INERTIA =                            */ 1100.0,              // kg⋅m² (estimated for IndyCar)
    /*CG_FROM_FRONT =                          */ 1.3,                 // m (43% of wheelbase, typical for IndyCar)
    /*CG_FROM_REAR = WHEELBASE - CG_FROM_FRONT */ 3.048 - 1.3,         // m

    /*TIRE_FORCE_SCALE*/ 13000.0,   // Newtons per game unit (estimate)
    /*MAX_GAME_FORCE_UNITS*/ 4000.0 // Maximum expected force in game units
);

static GameConstants NASCAR(
    /*VEHICLE_MASS =                           */ 1400.0 + 80.0 + 76.0, // kg (heavier stock car + fuel + driver)
    /*FRONT_TRACK =                            */ 1.524,                // m (60 inches, typical NASCAR)
    /*REAR_TRACK =                             */ 1.524,                // m (same as front for NASCAR)
    /*WHEELBASE =                              */ 2.794,                // m (110 inches, typical NASCAR)
    /*YAW_INERTIA =                            */ 2000.0,               // kg⋅m² (higher for heavier NASCAR car)
    /*CG_FROM_FRONT =                          */ 1.47,                 // m (52.5% of wheelbase, NASCAR is more rear-biased)
    /*CG_FROM_REAR = WHEELBASE - CG_FROM_FRONT */ 2.794 - 1.47,         // m

    /*TIRE_FORCE_SCALE*/ 13000.0,   // Newtons per game unit (estimate)
    /*MAX_GAME_FORCE_UNITS*/ 4000.0 // Maximum expected force in game units
);
} // namespace VehicleConstants

// Select constants based on game

static GameConstants GetGameConstants(GameVersion version)
{
    if (version == ICR2_DOS || version == ICR2_RENDITION || version == ICR2_WINDOWS)
    {
        // Use IndyCar constants
        return VehicleConstants::IndyCar;
    }
    else if (version == NASCAR1 || version == NASCAR2)
    {
        // Use NASCAR constants
        return VehicleConstants::NASCAR;
    }
    else
    {
        // Default to IndyCar if unknown
        return VehicleConstants::IndyCar;
    }
}

// Helper function to convert raw tire data to usable data
static double ConvertTireForceToNewtons(double tire_force_raw, const GameConstants& constants)
{
    // DON'T remove the sign - preserve it!
    return tire_force_raw * constants.TIRE_FORCE_SCALE / constants.MAX_GAME_FORCE_UNITS;
}

static int GetTurnDirection(double lf, double rf, double lr, double rr)
{
    // Determine if we're turning left or right based on force signs
    // Most of your forces will have the same sign during a turn
    int negative_count = 0;
    if (lf < 0) { negative_count++; }
    if (rf < 0) { negative_count++; }
    if (lr < 0) { negative_count++; }
    if (rr < 0) { negative_count++; }

    if (negative_count >= 2)
    {
        return -1; // Left turn (negative forces dominant)
    }
    else
    {
        return 1; // Right turn (positive forces dominant)
    }
}

bool CalculatedVehicleDynamics::Calculate(const RawTelemetry& current, RawTelemetry& /*previous*/, GameVersion version)
{
    GameConstants constants = GetGameConstants(version);

    // Convert units
    const double speed_ms = current.speed_mph * MPH_TO_MS;

    // Convert steering wheel angle to actual wheel angle
    // Typical IndyCar steering ratio is around 12:1 to 15:1
    const double STEERING_RATIO  = 15.0;
    const double wheel_angle_deg = current.steering_deg / STEERING_RATIO;
    const double wheel_angle_rad = wheel_angle_deg * M_PI / 180.0;

    // Force Assignments
    force_lf     = current.tiremaglat_lf;
    force_rf     = current.tiremaglat_rf;
    force_lr     = current.tiremaglat_lr;
    force_rr     = current.tiremaglat_rr;

    forceLong_lf = current.tiremaglong_lf;
    forceLong_rf = current.tiremaglong_rf;
    forceLong_lr = current.tiremaglong_lr;
    forceLong_rr = current.tiremaglong_rr;

    // Convert tire forces to "actual" Newtons
    // In the future if we find real forces we can replace this
    const double force_lf_N     = ConvertTireForceToNewtons(force_lf, constants);
    const double force_rf_N     = ConvertTireForceToNewtons(force_rf, constants);
    const double force_lr_N     = ConvertTireForceToNewtons(force_lr, constants);
    const double force_rr_N     = ConvertTireForceToNewtons(force_rr, constants);

    const double forceLong_lf_N = ConvertTireForceToNewtons(forceLong_lf, constants);
    const double forceLong_rf_N = ConvertTireForceToNewtons(forceLong_rf, constants);
    const double forceLong_lr_N = ConvertTireForceToNewtons(forceLong_lr, constants);
    const double forceLong_rr_N = ConvertTireForceToNewtons(forceLong_rr, constants);

    frontLeftForce_N            = force_lf_N;
    frontRightForce_N           = force_rf_N;

    frontLeftLong_N             = forceLong_lf_N;
    frontRightLong_N            = forceLong_rf_N;

    // CALC 1
    //===== LATERAL FORCE CALC ======

    // Calculate sum of lateral force
    const double total_lateral_force_N = force_lf_N + force_rf_N + force_lr_N + force_rr_N;

    // Determine turn direction and apply sign
    const int turn_direction = GetTurnDirection(force_lf, force_rf, force_lr, force_rr);

    // Calculate lateral acceleration: F = ma, so a = F/m
    const double lateral_acceleration = (total_lateral_force_N * turn_direction) / constants.VEHICLE_MASS;

    // Calculate total lateral force (maybe unneeded)
    totalLateralForce = force_lf + force_rf + force_lr + force_rr;

    // Convert to G-force
    lateralG = lateral_acceleration / VehicleConstants::GRAVITY;

    // Calculate direction value
    // Left  (positive totalLateralForce)
    // Right (negative totalLateralForce)
    directionVal = std::abs(lateralG) < 0.05 ? 0 : static_cast<int>(sign(totalLateralForce));

    // CALC 2
    //===== SLIP ANGLE ======

    // This is to cut out random noise
    if (speed_ms > STANDSTILL_SPEED && std::abs(lateralG) > MIN_LAT_G)
    { // Only calculate when moving and turning

        // Calculate front and rear lateral forces
        const double front_lateral_force = force_lf_N + force_rf_N;
        const double rear_lateral_force  = force_lr_N + force_rr_N;

        // Compare actual vs expected lateral acceleration
        double expected_lateral_accel = 0.0;
        if (std::abs(wheel_angle_rad) > 0.001)
        {
            // Expected lateral acceleration from steering input
            expected_lateral_accel = (speed_ms * speed_ms * std::tan(std::abs(wheel_angle_rad))) / constants.WHEELBASE;
        }

        const double actual_lateral_accel = std::abs(lateralG) * VehicleConstants::GRAVITY;

        // Response ratio
        const double response_ratio = expected_lateral_accel > 0.1 ? actual_lateral_accel / expected_lateral_accel : 1.0;

        // Front vs rear force balance
        const double total_force = std::abs(front_lateral_force + rear_lateral_force);
        // Positive = rear working harder (oversteer), Negative = front working harder (understeer)
        const double force_balance = total_force > 100.0 ? (std::abs(rear_lateral_force) - std::abs(front_lateral_force)) / total_force : 0.0;

        // Method 3: Left/right tire imbalance (detects if one end is sliding)
        const double front_imbalance = std::abs(std::abs(force_lf_N) - std::abs(force_rf_N)) / std::max(1.0, std::abs(force_lf_N + force_rf_N));
        const double rear_imbalance  = std::abs(std::abs(force_lr_N) - std::abs(force_rr_N)) / std::max(1.0, std::abs(force_lr_N + force_rr_N));

        // Higher imbalance = more sliding at that axle
        const double axle_slip_difference = rear_imbalance - front_imbalance;

        // Combine methods to get slip indicator
        double slip_indicator = 0.0;

        // Response method: too much response = oversteer, too little = understeer
        // Tune this to make slip match reality
        if (response_ratio > 1.0 + RESPONSE_THRESHOLD)
        {
            slip_indicator += (response_ratio - 1.0) * 0.5; // Oversteer
        }
        else if (response_ratio < 1.0 - RESPONSE_THRESHOLD)
        {
            slip_indicator += (response_ratio - 1.0) * 0.5; // Understeer (negative)
        }

        // Force balance method
        slip_indicator += force_balance * 0.5;

        // Axle imbalance method
        slip_indicator += axle_slip_difference * 0.3;

        // Convert to degrees and apply proper signs
        double slip_degrees = slip_indicator;

        // Apply turn-specific sign convention:
        // For LEFT turns: positive = oversteer, negative = understeer
        // For RIGHT turns: negative = oversteer, positive = understeer

        if (turn_direction > 0)
        {                                 // Right turn
            slip_degrees = -slip_degrees; // Flip signs for right turns
        }
        // For left turns (turn_direction < 0), keep signs as calculated

        slip = slip_degrees;
    }
    else
    {
        slip = 0.0; // No slip when not moving or turning
    }

    // Apply bounds to outputs
    lateralG = std::clamp(lateralG, -MAX_USEFUL_G, MAX_USEFUL_G); // Reasonable G range for IndyCar
    //yaw = std::clamp(yaw, -180.0, 180.0); // Limit yaw acceleration
    slip = std::clamp(slip, -45.0, 45.0); // Limit slip angle

    // Calculate force magnitude for FFB
    // Scale based on absolute lateral G, with max at 4G (typical for IndyCar cornering)
    const double gForceScale = saturate(std::abs(lateralG) / TYPICAL_G);

    // Apply speed scaling (reduce forces at low speeds like your other calculations)
    const double speedScale = saturate((current.speed_mph - SPEED_THRESHOLD) / SPEED_SCALE_RAMP_RANGE);

    forceMagnitude          = gForceScale * speedScale * DEFAULT_DINPUT_GAIN;

    // Store basic telemetry for output
    speedMph    = current.speed_mph;
    steeringDeg = current.steering_deg;

    return true;
}
