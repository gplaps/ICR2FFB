#pragma once
#include "calculations/slip_angle.h"
#include "calculations/lateral_load.h"
#include <dinput.h>
#include <string>

// Include logging
void LogMessage(const std::wstring& msg);

extern IDirectInputEffect* constantForceEffect;

void ApplyConstantForceEffect(
    const RawTelemetry& current,
    const RawTelemetry& previous,
    const CalculatedLateralLoad& load,
    const CalculatedSlip& slip,
    double speed_mph,
    IDirectInputEffect* constantForceEffect,
    double masterForceScale);