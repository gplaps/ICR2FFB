#pragma once
#include "project_dependencies.h"

#include "ffb_config.h"

#include <dinput.h>

struct FFBDevice
{
    explicit FFBDevice(const FFBConfig& configIn);
    FFBDevice()                   
    #if defined(IS_CPP11_COMPLIANT)
    = delete
    #endif
    ;
    IDirectInputDevice8* diDevice;

    // === Force Effect Creators ===
    void CreateConstantForceEffect();
    void CreateDamperEffect();
    void CreateSpringEffect();

    // === Force Effect Start ===
    void StartConstant();
    void StartDamper();
    void StartSpring();

    // === Force Effect Update ===
    void UpdateDamperEffect(LONG damperStrength);
    void UpdateSpringEffect(LONG springStrength);
    void UpdateConstantForceEffect(LONG magnitude, bool withDirection);

    int InitDevice();
    int DirectInputSetup() const;

    void Start();
    void Update();
    void Poll();

    const FFBConfig& config;

private:
    DIJOYSTATE2 js; // this is the read state of all axis and buttons

    // I think this is how we tell it these things are DirectInput stuff?
    IDirectInputEffect* constantForceEffect ;
    IDirectInputEffect* damperEffect        ;
    IDirectInputEffect* springEffect        ;

    bool constantStarted                    ;
    bool damperStarted                      ;
    bool springStarted                      ;
};
