#pragma once

struct DamperEffect {
    static double LowSpeedDamperStrength(double speedMph);
    static double Calculate(double speedMph, double masterForceScale, double damperForceScale);
};
