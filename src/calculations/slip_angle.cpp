#include "slip_angle.h"

#include "constants.h"
#include "helpers.h"

#include <cmath>

#include <algorithm>
// #include <deque>
#if !defined(IS_CPP11_COMPLIANT)
#    include <stdint.h>
#endif

// Some variables for smoothing
// const int VELOCITY_HISTORY_SIZE = 5;
// static std::deque<double> velocityAngleHistory;

struct DecodedSlip
{
    DecodedSlip(double sgnNorm, double mag) :
        signedNorm(sgnNorm),
        magnitude(mag) {}
    double signedNorm; // -1.0 to +1.0
    double magnitude;  // 0.0 to 1.0
};

// Just based on observation
// It seems the raw tires never really go above 4,000 'units' of whatever they are, so we assume this is max
static DecodedSlip decodeSlip(uint16_t raw)
{
    const double MAX_EXPECTED_SLIP = 4000.0;

    const bool    right            = raw > 0x8000;
    const int32_t mag              = right ? (0x10000 - raw) : raw;
    double        normalized       = static_cast<double>(mag) / MAX_EXPECTED_SLIP;
    normalized                     = saturate(normalized);
    const double signedNorm        = right ? -normalized : normalized;
    return DecodedSlip(signedNorm, normalized);
}

bool CalculatedSlip::Calculate(const RawTelemetry& current, RawTelemetry& /*previous*/)
{
    // Decode tire slip values
    const DecodedSlip lf = decodeSlip(static_cast<uint16_t>(current.tireload_lf));
    const DecodedSlip rf = decodeSlip(static_cast<uint16_t>(current.tireload_rf));
    const DecodedSlip lr = decodeSlip(static_cast<uint16_t>(current.tireload_lr));
    const DecodedSlip rr = decodeSlip(static_cast<uint16_t>(current.tireload_rr));

    slipNorm_lf          = lf.signedNorm;
    slipNorm_rf          = rf.signedNorm;
    slipNorm_lr          = lr.signedNorm;
    slipNorm_rr          = rr.signedNorm;

    slipMag_lf           = lf.magnitude;
    slipMag_rf           = rf.magnitude;
    slipMag_lr           = lr.magnitude;
    slipMag_rr           = rr.magnitude;

    // Compute average slip at front and rear
    const double frontSlip = (lf.signedNorm + rf.signedNorm) * 0.5;
    const double rearSlip  = (lr.signedNorm + rr.signedNorm) * 0.5;

    // Assume neutral slip when rear is 50% of front
    // Big assumption and has a massive affect on feel
    // Finding a real slip parameter would help a lot here
    // Based on observation of raw tire data it seems rears are like ~50% lower than fronts in mostly normal conditions
    // If the rears get close to front or exceed front you are definitely spinning
    const double rearBiasCorrection = 0.6; // Tune between 0.5–0.8 based on testing
    const double adjustedFrontSlip  = frontSlip * rearBiasCorrection;
    const double slipDelta          = rearSlip - adjustedFrontSlip; // Positive = oversteer, Negative = understeer

    // Normalize and scale
    const double slipAngleDeg = std::clamp(slipDelta * 180.0, -90.0, 90.0);

    // Compute output force scaling
    const double absSlipDeg_ = std::abs(slipAngleDeg);
    const double slipScale   = saturate((current.speed_mph - SPEED_THRESHOLD) / SLIP_SPEED_RAMP_RANGE);
    const double force       = saturate(absSlipDeg_ * 400.0 * slipScale);
    (void)force; // unused currently

    // Output
    slipAngle      = slipAngleDeg;
    absSlipDeg     = absSlipDeg_;
    forceMagnitude = saturate(absSlipDeg / 90.0);
    directionVal   = static_cast<int>(-sign(slipAngleDeg));

    return true;
}
