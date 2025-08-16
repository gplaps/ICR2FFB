#include "ffb_output.h"
#include "constants.h"
#include "helpers.h"
#include "log.h"
#include "constant_force.h"
#include "damper_effect.h"
#include "spring_effect.h"
#include <algorithm>

FFBOutput::FFBOutput(const FFBConfig& config) : device(config) { 
    if(!Init(config))
        return;
    mInitialized = true;
}

FFBOutput::~FFBOutput() {}

bool FFBOutput::Valid() const {
     return mInitialized;
}

int FFBOutput::ApplyFFBSettings(const FFBConfig& config) {
    // Parse FFB effect toggles from config <- should all ffb types be enabled? Allows user to select if they dont like damper for instance
    // Would be nice to add a % per effect in the future
    enableRateLimit = ToLower(config.targetLimitEnabled) == L"true";
    enableConstantForce = ToLower(config.targetConstantEnabled) == L"true";
    enableWeightForce = ToLower(config.targetWeightEnabled) == L"true";
    enableDamperEffect = ToLower(config.targetDamperEnabled) == L"true";
    enableSpringEffect = ToLower(config.targetSpringEnabled) == L"true";

    invert = ToLower(config.targetInvertFFB) == L"true";

    // Create FFB effects as needed
    if (enableConstantForce) device.CreateConstantForceEffect();
    if (enableDamperEffect)  device.CreateDamperEffect();
    if (enableSpringEffect)  device.CreateSpringEffect();

    // This is to control the max % for any of the FFB effects as specified in the ffb.ini
    // Prevents broken wrists (hopefully)
    masterForceValue = std::stod(config.targetForceSetting);
    constantForceValue = std::stod(config.targetConstantScale);
    weightForceValue = std::stod(config.targetWeightScale);
    damperForceValue = std::stod(config.targetDamperScale);

    // Master force scale -> Keeping Hands Safe
    masterForceScale = saturate(masterForceValue / 100.0);
    deadzoneForceScale = saturate(deadzoneForceValue / 100.0);
    constantForceScale = saturate(constantForceValue / 100.0);
    weightForceScale = saturate(weightForceValue / 100.0);
    damperForceScale = saturate(damperForceValue / 100.0);
    
    return 0;
}

int FFBOutput::Init(const FFBConfig& config) {
    int res;
    STATUS_CHECK(device.InitDevice());
    STATUS_CHECK(ApplyFFBSettings(config));
    return res;
}

void FFBOutput::Start() {
    device.Start();

    // Start damper/spring effects once telemetry is valid
    // Probably need to also figure out how to stop these when the game pauses
    // Also need to maybe fade in and out the effects when waking/sleeping
    if(enableDamperEffect) device.StartDamper();
    if(enableSpringEffect) device.StartSpring();
}

void FFBOutput::Update() {
    device.Update();
}

void FFBOutput::Poll() {
    device.Poll();
}
