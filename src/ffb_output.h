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
    bool enableRateLimit     = false;
    bool enableConstantForce = false;
    bool enableWeightForce   = false;
    bool enableDamperEffect  = false;
    bool enableSpringEffect  = false;

    bool invert              = false;

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
    double masterForceValue   = 0.0;
    double constantForceValue = 0.0;
    double deadzoneForceValue = 0.0;
    double weightForceValue   = 0.0;
    double damperForceValue   = 0.0;

    double masterForceScale   = 0.0;
    double deadzoneForceScale = 0.0;
    double constantForceScale = 0.0;
    double weightForceScale   = 0.0;
    double damperForceScale   = 0.0;

    bool mInitialized         = false;
};
