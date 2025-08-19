#pragma once

struct DamperEffect
{
    double LowSpeedDamperStrength(double speedMph);
    double Calculate(double speedMph, double masterForceScale, double damperForceScale);
};
