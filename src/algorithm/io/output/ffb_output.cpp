#include "ffb_output.h"

#include "log.h"
#include "math_utilities.h"
#include "safety_check.h"

#include <cmath>

FFBOutput::FFBOutput(const FFBConfig& config) :
    steeringDevice(config),
    pedals(config),
    masterForceScale(),
    constantForceScale(),
    damperForceScale(),
    springForceScale(),
    mInitialized(false)
{
    if (!Init(config))
    {
        return;
    }
    mInitialized = true;
}

FFBOutput::~FFBOutput() {}

bool FFBOutput::Valid() const
{
    return mInitialized;
}

bool FFBOutput::Init(const FFBConfig& config)
{
    // This is to control the max % for any of the FFB effects as specified in the ffb.ini
    // Prevents broken wrists (hopefully)
    masterForceScale   = saturate(config.GetDouble(L"effects", L"force") / 100.0);

    constantForceScale = saturate(config.GetDouble(L"effects", L"constant scale") / 100.0);
    damperForceScale   = saturate(config.GetDouble(L"effects", L"damper scale") / 100.0);
    springForceScale   = 1.0; // not configurable - route full effect, enabled flag considered in ffb_device

    // checks for user input values resulting in NaN
    SAFETY_CHECK(masterForceScale);
    SAFETY_CHECK(constantForceScale);
    SAFETY_CHECK(damperForceScale);
    SAFETY_CHECK(springForceScale);

    return steeringDevice.Valid() /*&& pedals.Valid()*/;
}

void FFBOutput::Start()
{
    steeringDevice.Start();
    // pedals.Start();
    // init of damper/spring done in steeringDevice.Start()

    // Start damper/spring effects once telemetry is valid
    // Probably need to also figure out how to stop these when the game pauses
    // Also need to maybe fade in and out the effects when waking/sleeping
}

void FFBOutput::Update(double constant, double damper, double spring, bool paused)
{
    double constantOut = constant * constantForceScale * masterForceScale;
    double damperOut   = damper * damperForceScale * masterForceScale;
    double springOut   = spring * springForceScale * masterForceScale;

    SAFETY_CHECK(constantOut);
    SAFETY_CHECK(damperOut);
    SAFETY_CHECK(springOut);

    steeringDevice.Update(constantOut, damperOut, springOut, !paused);
    // pedals.Update(constantOut, damperOut, springOut, false);
}

void FFBOutput::Poll()
{
    steeringDevice.Poll();
    // pedals.Poll();
}
