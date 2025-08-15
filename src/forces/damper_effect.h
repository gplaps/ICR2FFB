#pragma once
#include "project_dependencies.h"
#include <string>

double LowSpeedDamperStrength(double speedMph);
void UpdateDamperEffect(double speedMph, IDirectInputEffect* effect, double masterForceScale, double damperForceScale);
