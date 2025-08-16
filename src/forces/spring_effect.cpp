#include "spring_effect.h"
#include "constants.h"
#include "project_dependencies.h"
#include <iostream>
#include <algorithm>

// just basic centering spring to try to give the wheel more weight while driving
// Used to scale to speed but ive never found this effect to feel very nice on the fanatec
void SpringEffect::Update(FFBDevice& device, double masterForceScale) {
    // How much centering force?
    const LONG springStrength = static_cast<LONG>(6500.0 * masterForceScale);
    device.UpdateSpringEffect(springStrength);
}
