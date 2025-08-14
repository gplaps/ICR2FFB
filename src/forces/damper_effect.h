#pragma once
#include <dinput.h>
#include <string>

// Include logging
void LogMessage(const std::wstring& msg);

void UpdateDamperEffect(double speedMph, IDirectInputEffect* effect, double masterForceScale, double damperForceScale);
