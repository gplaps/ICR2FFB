#pragma once


#include "constant_force.h"
#include "ffb_steering_device.h"

struct FFBOutput
{
    explicit FFBOutput(const FFBConfig& config);
    virtual ~FFBOutput();

    FFBSteeringDevice steeringDevice;
    // leave option for further devices, like pedal vibrations

    // === Force Feedback Flags & States ===
    // I have 3 effects right now which all get calculated separately
    // I think probably all you need are these three to make good FFB

    // Constant force is the most in depth
    // Damper & Spring just use speed to do things

    int  ApplyFFBSettings(const FFBConfig& config);
    bool Init(const FFBConfig& config);
    bool Valid() const;

    void Start();
    void Update();
    void Poll();

    // expects [0-1] scaled values
    void Update(double constantStrength, double damperStrength, double springStrength, bool paused);

    // Master force scale -> Keeping Hands Safe
    double masterForceScale;

    double constantForceScale;
    double damperForceScale;
    double springForceScale;

    bool mInitialized;
};
