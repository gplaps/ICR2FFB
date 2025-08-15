#pragma once
#include <dinput.h>
#include <string>

double LowSpeedDamperStrength(double speedMph);
void UpdateDamperEffect(double speedMph, IDirectInputEffect* effect, double masterForceScale, double damperForceScale);
