#pragma once

#include "ffb_config.h"

struct SpringEffect
{
    explicit SpringEffect(const FFBConfig& config);

    double Calculate(double input) const;

    double springStrength;
};
