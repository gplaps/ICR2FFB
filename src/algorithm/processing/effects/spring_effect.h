#pragma once

struct SpringEffect
{
    explicit SpringEffect();

    double Calculate() const;

    double springStrength;
};
