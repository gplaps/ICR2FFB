#include "ffb_output.h"

#include "math_utilities.h"
#include "utilities.h"

FFBOutput::FFBOutput(const FFBConfig& config) :
    steeringDevice(config),
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
    springForceScale   = 1.0; // not configurable

    return true; // evaluate if this can fail
}

void FFBOutput::Start()
{
    steeringDevice.Start();
    // init of damper/spring done in steeringDevice.Start()

    // Start damper/spring effects once telemetry is valid
    // Probably need to also figure out how to stop these when the game pauses
    // Also need to maybe fade in and out the effects when waking/sleeping
}

void FFBOutput::Update(double constant, double damper, double spring, bool paused)
{
    steeringDevice.Update(constant * constantForceScale * masterForceScale, damper * damperForceScale * masterForceScale, spring /* * springForceScale*/ * masterForceScale, !paused);
}

void FFBOutput::Poll()
{
    steeringDevice.Poll();
}
