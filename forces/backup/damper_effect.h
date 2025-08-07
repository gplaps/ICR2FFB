#pragma once
#include <dinput.h>

extern IDirectInputEffect* damperEffect;

void UpdateDamperEffect(double speedMph, IDirectInputEffect* effect);