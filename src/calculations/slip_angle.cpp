#include "slip_angle.h"
#include "constants.h"
#include "helpers.h"
#include <cmath>
#include <algorithm>
#include <deque>

// Some variables for smoothing
// const int VELOCITY_HISTORY_SIZE = 5;
static std::deque<double> velocityAngleHistory;

struct DecodedSlip {
    double signedNorm;  // -1.0 to +1.0
    double magnitude;   // 0.0 to 1.0
};

// Just based on observation
// It seems the raw tires never really go above 4,000 'units' of whatever they are, so we assume this is max
static DecodedSlip decodeSlip(uint16_t raw) {
    const double MAX_EXPECTED_SLIP = 4000.0;

    bool right = raw > 0x8000;
    int32_t mag = right ? (0x10000 - raw) : raw;
    double normalized = static_cast<double>(mag) / MAX_EXPECTED_SLIP;
    normalized = std::clamp(normalized, 0.0, 1.0);
    double signedNorm = right ? -normalized : normalized;
    return { signedNorm, normalized };
}

bool CalculateSlipAngle(const RawTelemetry& current, RawTelemetry& /*previous*/, CalculatedSlip& out) {
    // Decode tire slip values
    DecodedSlip lf = decodeSlip(static_cast<uint16_t>(current.tireload_lf));
    DecodedSlip rf = decodeSlip(static_cast<uint16_t>(current.tireload_rf));
    DecodedSlip lr = decodeSlip(static_cast<uint16_t>(current.tireload_lr));
    DecodedSlip rr = decodeSlip(static_cast<uint16_t>(current.tireload_rr));

    out.slipNorm_lf = lf.signedNorm;
    out.slipNorm_rf = rf.signedNorm;
    out.slipNorm_lr = lr.signedNorm;
    out.slipNorm_rr = rr.signedNorm;

    out.slipMag_lf = lf.magnitude;
    out.slipMag_rf = rf.magnitude;
    out.slipMag_lr = lr.magnitude;
    out.slipMag_rr = rr.magnitude;

    // Compute average slip at front and rear
    double frontSlip = (lf.signedNorm + rf.signedNorm) * 0.5;
    double rearSlip = (lr.signedNorm + rr.signedNorm) * 0.5;

    // Assume neutral slip when rear is 50% of front
    // Big assumption and has a massive affect on feel
    // Finding a real slip parameter would help a lot here
    // Based on observation of raw tire data it seems rears are like ~50% lower than fronts in mostly normal conditions
    // If the rears get close to front or exceed front you are definitely spinning
    const double rearBiasCorrection = 0.6;  // Tune between 0.5–0.8 based on testing
    double adjustedFrontSlip = frontSlip * rearBiasCorrection;
    double slipDelta = rearSlip - adjustedFrontSlip;  // Positive = oversteer, Negative = understeer

    // Normalize and scale
    double slipAngleDeg = std::clamp(slipDelta * 180.0, -90.0, 90.0);

    // Compute output force scaling
    double absSlipDeg = std::abs(slipAngleDeg);
    double slipScale = std::clamp((current.speed_mph - SPEED_THRESHOLD) / SLIP_SPEED_RAMP_RANGE, 0.0, 1.0);
    double force = std::clamp(absSlipDeg * 400.0 * slipScale, 0.0, 1.0);
    (void)force; // unused currently

    // Output
    out.slipAngle = slipAngleDeg;
    out.absSlipDeg = std::abs(slipAngleDeg);
    out.forceMagnitude = std::clamp(out.absSlipDeg / 90.0, 0.0, 1.0);
    out.directionVal = (slipAngleDeg > 0) ? -1 : (slipAngleDeg < 0 ? 1 : 0);

    return true;
}
