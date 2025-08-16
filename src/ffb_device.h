#pragma once
#include "project_dependencies.h"
#include "ffb_config.h"
#include <dinput.h>

struct FFBDevice {
    FFBDevice(const FFBConfig& config);
    FFBDevice()=delete;
    IDirectInputDevice8* matchedDevice = NULL;

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
    int DirectInputSetup();

    void Start();
    void Update();
    void Poll();

    const FFBConfig& config;

private:
    DIJOYSTATE2 js; // this is the read state of all axis and buttons

    // I think this is how we tell it these things are DirectInput stuff?
    IDirectInputEffect* constantForceEffect = NULL;
    IDirectInputEffect* damperEffect = NULL;
    IDirectInputEffect* springEffect = NULL;

    bool constantStarted = false;
    bool damperStarted = false;
    bool springStarted = false;
};
