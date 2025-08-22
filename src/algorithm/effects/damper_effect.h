#pragma once
#include "ffb_config.h"

struct DamperEffect
{
    explicit DamperEffect(const FFBConfig& config);
    double LowSpeedDamperStrength(double speedMph) const;
    double Calculate(double speedMph) const;

    double maxDamper;
    double maxSpeed;
};
