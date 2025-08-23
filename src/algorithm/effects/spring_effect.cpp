#include "spring_effect.h"

#include "constants.h"
#include "log.h" // IWYU pragma: keep
#include "safety_check.h"

// just basic centering spring to try to give the wheel more weight while driving
// Used to scale to speed but ive never found this effect to feel very nice on the fanatec

SpringEffect::SpringEffect(const FFBConfig& config) :
    springStrength(config.GetDouble(L"effects", L"spring strength"))
{
    SAFETY_CHECK(springStrength);
}

double SpringEffect::Calculate() const
{
    // How much centering force?
    return springStrength / DEFAULT_DINPUT_GAIN_DBL;
}
