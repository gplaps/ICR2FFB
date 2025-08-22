#include "ffb_steering_device.h"

#include "ffb_config.h"
#include "math_utilities.h"

FFBSteeringDevice::FFBSteeringDevice(const FFBConfig& config) :
    FFBDevice(config, config.GetString(L"base", L"device")),
    invert()
{
    Init(config);
}

FFBSteeringDevice::~FFBSteeringDevice() {}

void FFBSteeringDevice::Init(const FFBConfig& config)
{
    invert = config.GetBool(L"effects", L"invert");
}

void FFBSteeringDevice::Update(double constantStrength, double damperStrength, double springStrength, bool constantWithDirection)
{
    FFBDevice::Update(invert ? -constantStrength : constantStrength, damperStrength, springStrength, constantWithDirection);
}
