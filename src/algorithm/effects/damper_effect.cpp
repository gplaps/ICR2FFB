#include "damper_effect.h"

#include "log.h"
#include "math_utilities.h"

// Create damper to make it feel like the steering is not powered, mostly for pitlane, maybe hairpin use
DamperEffect::DamperEffect(const FFBConfig& config) :
    maxDamper(5000.0),                                      // consider making configurable, replacing damper scale setting
    maxSpeed(config.GetDouble(L"effects", L"damper speed")) // Only goes to '40mph'
{
    if (maxSpeed <= 0.0) { maxSpeed = 40.0; } // div-by-zero
}

double DamperEffect::LowSpeedDamperStrength(double speedMph) const
{
    const double t = saturate(std::abs(speedMph) / maxSpeed);
    return t;
}

double DamperEffect::Calculate(double speedMph) const
{
    const double minDamper   = 0.0;
    const double damperScale = LowSpeedDamperStrength(speedMph);
    // linear could be replaced by exponential or otherwise shaped curve to model overcoming the tyre friction without rolling tyres which is super heavy at zero but falls of pretty quickly if tyres rotate - or outsource this into a second effect
    const double damperRaw = lerp(maxDamper, minDamper, damperScale);
    const double damper01  = damperRaw / maxDamper;
    return damper01;
}
