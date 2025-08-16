#include "damper_effect.h"
#include "constants.h"
#include "helpers.h"
#include "project_dependencies.h"
#include <iostream>
#include <algorithm>


// Create damper to make it feel like the steering is not powered, mostly for pitlane, maybe hairpin use
// Only goes to '40mph'
static const double maxDamper = 5000.0; // or move to constants.h
double DamperEffect::LowSpeedDamperStrength(double speedMph) {
    const double maxSpeed = 40.0;
    // const double minDamper = 0.0;

    const double t = saturate(speedMph / maxSpeed);
    return t;
}

void DamperEffect::Update(double damperScale, FFBDevice& device, double masterForceScale, double damperForceScale) {
    const LONG damperStrength = static_cast<LONG>(((1.0 - damperScale) * maxDamper * masterForceScale) * damperForceScale);
    device.UpdateDamperEffect(damperStrength);
}
