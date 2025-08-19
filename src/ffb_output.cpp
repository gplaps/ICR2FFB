#include "ffb_output.h"

#include "helpers.h"

FFBOutput::FFBOutput(const FFBConfig& config) :
    device(config),
    enableRateLimit(),
    enableConstantForce(),
    enableWeightForce(),
    enableDamperEffect(),
    enableSpringEffect(),
    invert(),
    damperEffect(),
    springEffect(),
    masterForceValue(),
    constantForceValue(),
    deadzoneForceValue(),
    brakingForceValue(),
    weightForceValue(),
    damperForceValue(),
    masterForceScale(),
    constantForceScale(),
    deadzoneForceScale(),
    brakingForceScale(),
    weightForceScale(),
    damperForceScale(),
    mInitialized(false)
{
    if (Init(config))
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

int FFBOutput::ApplyFFBSettings(const FFBConfig& config)
{
    // Parse FFB effect toggles from config <- should all ffb types be enabled? Allows user to select if they dont like damper for instance
    // Would be nice to add a % per effect in the future
    enableRateLimit     = config.GetBool(L"limit");
    enableConstantForce = config.GetBool(L"constant");
    enableWeightForce   = config.GetBool(L"weight");
    enableDamperEffect  = config.GetBool(L"damper");
    enableSpringEffect  = config.GetBool(L"spring");

    invert              = config.GetBool(L"invert");

    // Create FFB effects as needed
    if (enableConstantForce) { device.CreateConstantForceEffect(); }
    if (enableDamperEffect) { device.CreateDamperEffect(); }
    if (enableSpringEffect) { device.CreateSpringEffect(); }

    // This is to control the max % for any of the FFB effects as specified in the ffb.ini
    // Prevents broken wrists (hopefully)
    masterForceValue   = config.GetDouble(L"force");
    constantForceValue = config.GetDouble(L"constant scale");
    weightForceValue   = config.GetDouble(L"weight scale");
    deadzoneForceValue = config.GetDouble(L"deadzone");
    damperForceValue   = config.GetDouble(L"damper scale");
    brakingForceValue  = config.GetDouble(L"braking scale");

    // Master force scale -> Keeping Hands Safe
    masterForceScale   = saturate(masterForceValue / 100.0);
    deadzoneForceScale = saturate(deadzoneForceValue / 100.0);
    constantForceScale = saturate(constantForceValue / 100.0);
    weightForceScale   = saturate(weightForceValue / 100.0);
    damperForceScale   = saturate(damperForceValue / 100.0);

    brakingForceScale  = brakingForceValue;

    return 0;
}

int FFBOutput::Init(const FFBConfig& config)
{
    int res = 0;
    STATUS_CHECK(device.InitDevice());
    STATUS_CHECK(ApplyFFBSettings(config));
    return res;
}

void FFBOutput::Start()
{
    device.Start();

    // Start damper/spring effects once telemetry is valid
    // Probably need to also figure out how to stop these when the game pauses
    // Also need to maybe fade in and out the effects when waking/sleeping
    if (enableDamperEffect) { device.StartDamper(); }
    if (enableSpringEffect) { device.StartSpring(); }
}

void FFBOutput::Update()
{
    device.Update();
}

void FFBOutput::Poll()
{
    device.Poll();
}

void FFBOutput::UpdateConstantForce(const ConstantForceEffectResult& result)
{
    // Start constant force once telemetry is valid
    if (enableConstantForce)
    {
        device.StartConstant();

        device.UpdateConstantForceEffect(static_cast<LONG>(result.magnitude10000), result.paused);
    }
}

void FFBOutput::UpdateDamper(double speed_mph)
{
    if (enableDamperEffect)
    {
        const double damperStrength = damperEffect.Calculate(speed_mph, masterForceScale, damperForceScale);
        device.UpdateDamperEffect(static_cast<LONG>(damperStrength));
    }
}

void FFBOutput::UpdateSpring()
{
    if (enableSpringEffect)
    {
        const double springStrength = springEffect.Calculate(masterForceScale);
        device.UpdateSpringEffect(static_cast<LONG>(springStrength));
    }
}
