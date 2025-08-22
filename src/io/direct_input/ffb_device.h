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

    // expected range is [0-1] for strength values
    virtual void Update(double constantStrength, double damperStrength, double springStrength, bool constantWithDirection);

    void Start();
    void Poll();

private:
    bool InitDevice(const std::wstring& productNameOrIndex);
    void InitEffects(const FFBConfig& config);
    bool DirectInputSetup() const;

    IDirectInputDevice8* diDevice;
    DIJOYSTATE2          js; // this is the read state of all axis and buttons

    DiConstantEffect* constant;
    DiDamperEffect*   damper;
    DiSpringEffect*   spring;

    bool mInitialized;
};
