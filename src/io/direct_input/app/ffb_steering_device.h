#pragma once
#include "ffb_device.h"

struct FFBSteeringDevice : public FFBDevice
{
    explicit FFBSteeringDevice(const FFBConfig& config);
    ~FFBSteeringDevice() OVERRIDE;

    void Update(double constantStrength, double damperStrength, double springStrength, bool constantWithDirection) OVERRIDE;

private:
    void Init(const FFBConfig& config);

    bool invert;
};
