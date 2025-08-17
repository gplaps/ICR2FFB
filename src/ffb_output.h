#pragma once

#include "project_dependencies.h"

#include "constant_force.h"
#include "damper_effect.h"
#include "ffb_device.h"
#include "spring_effect.h"

struct FFBOutput
{
    explicit FFBOutput(const FFBConfig& config);
    ~FFBOutput();

    FFBDevice device;

    // === Force Feedback Flags & States ===
    // I have 3 effects right now which all get calculated separately
    // I think probably all you need are these three to make good FFB

    // Constant force is the most in depth
    // Damper & Spring just use speed to do things
    bool enableRateLimit;
    bool enableConstantForce;
    bool enableWeightForce;
    bool enableDamperEffect;
    bool enableSpringEffect;

    bool invert;

    int  ApplyFFBSettings(const FFBConfig& config);
    int  Init(const FFBConfig& config);
    bool Valid() const;

    void Start();
    void Update();
    void Poll();

    void UpdateConstantForce(const ConstantForceEffectResult& result);
    void UpdateDamper(double speed_mph);
    void UpdateSpring();

    DamperEffect damperEffect;
    SpringEffect springEffect;

    // init with zero to be safe
    double masterForceValue;
    double constantForceValue;
    double deadzoneForceValue;
    double weightForceValue;
    double damperForceValue;

    double masterForceScale;
    double deadzoneForceScale;
    double constantForceScale;
    double weightForceScale;
    double damperForceScale;

    bool mInitialized;
};
