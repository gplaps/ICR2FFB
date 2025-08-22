#pragma once
#include "ffb_config.h"

struct DamperEffect
{
    DamperEffect(const FFBConfig& config);
    double LowSpeedDamperStrength(double speedMph) const;
    double Calculate(double speedMph) const;

    double maxDamper;
    double maxSpeed;
};
