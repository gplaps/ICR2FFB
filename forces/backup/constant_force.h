#pragma once
#include "calculations/slip_angle.h"
#include "calculations/lateral_load.h"
#include <dinput.h>

extern IDirectInputEffect* constantForceEffect;

void ApplyConstantForceEffect(const CalculatedLateralLoad& load, IDirectInputEffect* constantForceEffect);