#include "damper_effect.h"

#include "log.h"
#include "math_utilities.h"

// Create damper to make it feel like the steering is not powered, mostly for pitlane, maybe hairpin use
DamperEffect::DamperEffect(const FFBConfig& config) :
    maxDamper(5000.0),                                      // consider making configurable, replacing damper scale setting
    maxSpeed(config.GetDouble(L"effects", L"damper speed")) // Only goes to '40mph'
{
    if (maxSpeed <= 0.0 || !std::isfinite(maxSpeed)) { maxSpeed = 40.0; } // div-by-zero
}

double DamperEffect::LowSpeedDamperStrength(double speedMph) const
{
    const double t = saturate(std::abs(speedMph) / maxSpeed);
    return t;
}

static double cubicEasing(double t) { return t * t * t; }

double DamperEffect::Calculate(double speedMph) const
{
    const double minDamper   = 0.0;
    const double damperScale = LowSpeedDamperStrength(speedMph);
    const double damper01 = lerp(maxDamper, minDamper, damperScale) / maxDamper;
    // experiment with curve shapes - this should model working against tyre friction of big slick tires without them rolling which is super heavy at zero but falls of pretty quickly if tyres rotate
    const double damperCurve = saturate(cubicEasing(damper01));
    // LogMessage(std::to_wstring(damper01) + L" -> " + std::to_wstring(damperCurve));
    return damperCurve;
}
