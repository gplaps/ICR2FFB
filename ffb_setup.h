#pragma once

// === Windows & DirectInput ===
#include <windows.h>
#include <dinput.h>

// === Standard Includes ===
#include <string>

// === Forward Declarations ===
extern std::wstring targetDeviceName;
extern std::wstring targetGameVersion;
extern std::wstring targetGameWindowName;

extern std::wstring targetForceSetting;
extern std::wstring targetDamperEnabled;
extern std::wstring targetSpringEnabled;
extern std::wstring targetConstantEnabled;
extern std::wstring targetInvertFFB;

extern IDirectInputDevice8* matchedDevice;
extern LPDIRECTINPUT8 directInput;

// Include logging
void LogMessage(const std::wstring& msg);

// === FFB Setup Functions ===
bool LoadFFBSettings(const std::wstring& filename);
bool InitializeDevice();