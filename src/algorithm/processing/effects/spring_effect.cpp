#include "spring_effect.h"

// just basic centering spring to try to give the wheel more weight while driving
// Used to scale to speed but ive never found this effect to feel very nice on the fanatec

SpringEffect::SpringEffect() :
    springStrength(1.0) {}

double SpringEffect::Calculate() const
{
    // How much centering force?
    return springStrength;
}
