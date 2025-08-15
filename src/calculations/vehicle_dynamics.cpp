#include "vehicle_dynamics.h"
#include "constants.h"
#include "helpers.h"
#include <cmath>
#include <algorithm>

// SAE Convention
// Lateral force 
// Negative <---------> Positive

// Left Turn
// Lateral G -> Positive
// Oversteer Slip -> Positive
// Understeer Slip -> Negative


// Vehicle Constants
// Most of this is estimates/guesses
// May or may not be used
namespace VehicleConstants {
    const double VEHICLE_MASS = 700.0 + 60.0 + 76.0; // kg (car + fuel + driver) - Can add real fuel later
    // const double FRONT_TRACK = 1.753; // m
    // const double REAR_TRACK = 1.638; // m  
    const double WHEELBASE = 3.048; // m
    // const double YAW_INERTIA = 1100.0; // kg⋅m² (estimated for IndyCar)
    // const double CG_FROM_FRONT = 1.3; // m (43% of wheelbase, typical for IndyCar)
    // const double CG_FROM_REAR = WHEELBASE - CG_FROM_FRONT; // m
    const double GRAVITY = 9.81; // m/s²

    // Tire guesswork since we do not know precise load in Newtons
    // Will calibrate against Indianapolis
    // ~220mph turn should be about 4G
    const double TIRE_FORCE_SCALE = 13000.0; // Newtons per game unit (adjust based on testing)
    const double MAX_GAME_FORCE_UNITS = 4000.0; // Maximum expected force in game units

}

// Helper function to convert raw tire data to usable data
static double convertTireForceToNewtons(double tire_force_raw) {
    return tire_force_raw * VehicleConstants::TIRE_FORCE_SCALE / VehicleConstants::MAX_GAME_FORCE_UNITS;
}

static int getTurnDirection(double lf, double rf, double lr, double rr) {
    // Determine if we're turning left or right based on force signs
    // Most of your forces will have the same sign during a turn
    int negative_count = 0;
    if (lf < 0) negative_count++;
    if (rf < 0) negative_count++;
    if (lr < 0) negative_count++;
    if (rr < 0) negative_count++;

    if (negative_count >= 2) {
        return -1; // Left turn (negative forces dominant)
    }
    else {
        return 1;  // Right turn (positive forces dominant)
    }
}

bool CalculateVehicleDynamics(const RawTelemetry& current, RawTelemetry& previous, bool& firstReading, CalculatedVehicleDynamics& out) {
    if (firstReading) {
        previous = current;
        firstReading = false;
        return false;
    }

    // Convert units
    double speed_ms = current.speed_mph * 0.44704; // mph to m/s

    // Convert steering wheel angle to actual wheel angle
    // Typical IndyCar steering ratio is around 12:1 to 15:1
    const double STEERING_RATIO = 15.0;
    double wheel_angle_deg = current.steering_deg / STEERING_RATIO;
    double wheel_angle_rad = wheel_angle_deg * M_PI / 180.0;

    // Force Assignments
    out.force_lf = current.tiremaglat_lf;
    out.force_rf = current.tiremaglat_rf;
    out.force_lr = current.tiremaglat_lr;
    out.force_rr = current.tiremaglat_rr;

    // Convert tire forces to "actual" Newtons
    // In the future if we find real forces we can replace this
    double force_lf_N = convertTireForceToNewtons(out.force_lf);
    double force_rf_N = convertTireForceToNewtons(out.force_rf);
    double force_lr_N = convertTireForceToNewtons(out.force_lr);
    double force_rr_N = convertTireForceToNewtons(out.force_rr);

    out.frontLeftForce_N = convertTireForceToNewtons(out.force_lf);
    out.frontRightForce_N = convertTireForceToNewtons(out.force_rf);

    // CALC 1
    //===== LATERAL FORCE CALC ======
    
    // Calculate sum of lateral force
    double total_lateral_force_N = force_lf_N + force_rf_N + force_lr_N + force_rr_N;

    // Determine turn direction and apply sign
    int turn_direction = getTurnDirection(out.force_lf, out.force_rf, out.force_lr, out.force_rr);

    // Calculate lateral acceleration: F = ma, so a = F/m
    double lateral_acceleration = (total_lateral_force_N * turn_direction) / VehicleConstants::VEHICLE_MASS;

    // Calculate total lateral force (maybe unneeded)
    out.totalLateralForce = out.force_lf + out.force_rf + out.force_lr + out.force_rr;
    
    // Convert to G-force
    out.lateralG = lateral_acceleration / VehicleConstants::GRAVITY;

    // Calculate direction value
    if (std::abs(out.lateralG) < 0.05) {
        out.directionVal = 0; // Straight
    }
    else {
        out.directionVal = sign(out.totalLateralForce);
    }

    // CALC 2
    //===== SLIP ANGLE ======

    // This is to cut out random noise
    if (speed_ms > STANDSTILL_SPEED && std::abs(out.lateralG) > MIN_LAT_G) { // Only calculate when moving and turning

        // Calculate front and rear lateral forces
        double front_lateral_force = force_lf_N + force_rf_N;
        double rear_lateral_force = force_lr_N + force_rr_N;

        // Compare actual vs expected lateral acceleration
        double expected_lateral_accel = 0.0;
        if (std::abs(wheel_angle_rad) > 0.001) {
            // Expected lateral acceleration from steering input
            expected_lateral_accel = (speed_ms * speed_ms * std::tan(std::abs(wheel_angle_rad))) / VehicleConstants::WHEELBASE;
        }

        double actual_lateral_accel = std::abs(out.lateralG) * VehicleConstants::GRAVITY;

        // Response ratio
        double response_ratio = 1.0;
        if (expected_lateral_accel > 0.1) {
            response_ratio = actual_lateral_accel / expected_lateral_accel;
        }

        // Front vs rear force balance
        double total_force = std::abs(front_lateral_force + rear_lateral_force);
        double force_balance = 0.0;
        if (total_force > 100.0) {
            // Positive = rear working harder (oversteer), Negative = front working harder (understeer)
            force_balance = (std::abs(rear_lateral_force) - std::abs(front_lateral_force)) / total_force;
        }

        // Method 3: Left/right tire imbalance (detects if one end is sliding)
        double front_imbalance = std::abs(std::abs(force_lf_N) - std::abs(force_rf_N)) / std::max(1.0, std::abs(force_lf_N + force_rf_N));
        double rear_imbalance = std::abs(std::abs(force_lr_N) - std::abs(force_rr_N)) / std::max(1.0, std::abs(force_lr_N + force_rr_N));

        // Higher imbalance = more sliding at that axle
        double axle_slip_difference = rear_imbalance - front_imbalance;

        // Combine methods to get slip indicator
        double slip_indicator = 0.0;

        // Response method: too much response = oversteer, too little = understeer
        // Tune this to make slip match reality
        if (response_ratio > 1.0 + RESPONSE_THRESHOLD) {
            slip_indicator += (response_ratio - 1.0) * 0.5; // Oversteer
        }
        else if (response_ratio < 1.0 - RESPONSE_THRESHOLD) {
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

        if (turn_direction > 0) { // Right turn
            slip_degrees = -slip_degrees; // Flip signs for right turns
        }
        // For left turns (turn_direction < 0), keep signs as calculated

        out.slip = slip_degrees;

    }
    else {
        out.slip = 0.0; // No slip when not moving or turning
    }

    // Apply bounds to outputs
    out.lateralG = std::clamp(out.lateralG, -MAX_USEFUL_G, MAX_USEFUL_G); // Reasonable G range for IndyCar
    //out.yaw = std::clamp(out.yaw, -180.0, 180.0); // Limit yaw acceleration
    out.slip = std::clamp(out.slip, -45.0, 45.0); // Limit slip angle

    // Calculate force magnitude for FFB
    // Scale based on absolute lateral G, with max at 4G (typical for IndyCar cornering)
    double gForceScale = std::clamp(std::abs(out.lateralG) / TYPICAL_G, 0.0, 1.0);

    // Apply speed scaling (reduce forces at low speeds like your other calculations)
    double speedScale = std::clamp((current.speed_mph - SPEED_THRESHOLD) / SPEED_SCALE_RAMP_RANGE, 0.0, 1.0);

    out.forceMagnitude = gForceScale * speedScale;

    // Store basic telemetry for output
    out.speedMph = current.speed_mph;
    out.steeringDeg = current.steering_deg;

    previous = current;
    return true;
}
