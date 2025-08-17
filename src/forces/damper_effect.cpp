#include "damper_effect.h"
#include "constants.h"
#include "helpers.h"


// Create damper to make it feel like the steering is not powered, mostly for pitlane, maybe hairpin use
// Only goes to '40mph'
static const double maxDamper = 5000.0;
double DamperEffect::LowSpeedDamperStrength(double speedMph) {
    const double maxSpeed = 40.0;
    // const double minDamper = 0.0;
    const double t = saturate(speedMph / maxSpeed);
    return t;
}

double DamperEffect::Calculate(double speedMph, double masterForceScale, double damperForceScale) {
    const double damperScale = LowSpeedDamperStrength(speedMph);
    return ((1.0 - damperScale) * maxDamper * masterForceScale) * damperForceScale;
}
