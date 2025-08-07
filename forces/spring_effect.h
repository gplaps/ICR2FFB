#pragma once
#include <dinput.h>
#include <string>

// Include logging
void LogMessage(const std::wstring& msg);

extern IDirectInputEffect* springEffect;

void UpdateSpringEffect(IDirectInputEffect* effect, double masterForceScale);