#pragma once

#include "ffb_config.h"

struct SpringEffect
{
    explicit SpringEffect();

    double Calculate() const;

    double springStrength;
};
