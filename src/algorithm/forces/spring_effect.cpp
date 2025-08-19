#include "spring_effect.h"

// just basic centering spring to try to give the wheel more weight while driving
// Used to scale to speed but ive never found this effect to feel very nice on the fanatec
double SpringEffect::Calculate(double masterForceScale)
{
    // How much centering force?
    const double springStrength = 6500.0 * masterForceScale;
    return springStrength;
}
