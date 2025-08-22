#pragma once
#include "project_dependencies.h" // IWYU pragma: keep

#include "ffb_config.h"
#include "ffb_effect.h"

#include <dinput.h>

struct FFBDevice
{
    explicit FFBDevice(const FFBConfig& config, const std::wstring& name);
    // explicit FFBDevice(IDirectInputDevice8* diPtr, const std::wstring& name);
#if defined(IS_CPP11_COMPLIANT)
    FFBDevice() = delete;
#else
    FFBDevice(); // intentionally declared but undefined -> linker error if used
#endif
    virtual ~FFBDevice();
    IDirectInputDevice8* diDevice;

    // expected range is [0-1] for strength values
    virtual void Update(double constantStrength, double damperStrength, double springStrength, bool constantWithDirection);

    void InitEffects(const FFBConfig& config);
    bool DirectInputSetup() const;

    void Start();
    void Poll();

private:
    DIJOYSTATE2 js; // this is the read state of all axis and buttons

    DiConstantEffect* constant;
    DiDamperEffect*   damper;
    DiSpringEffect*   spring;

    std::wstring productName;
    bool         mInitialized;
};
