#pragma once
#include "project_dependencies.h"
#include "ffb_device.h"
#include <string>

struct DamperEffect {
    static double LowSpeedDamperStrength(double speedMph);
    void Update(double speedMph, FFBDevice& device, double masterForceScale, double damperForceScale);
};
