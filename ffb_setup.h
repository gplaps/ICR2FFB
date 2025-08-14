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
extern std::wstring targetDeadzoneSetting;
extern std::wstring targetInvertFFB;
extern std::wstring targetLimitEnabled;
extern std::wstring targetConstantEnabled;
extern std::wstring targetConstantScale;
extern std::wstring targetWeightEnabled;
extern std::wstring targetWeightScale;
extern std::wstring targetDamperEnabled;
extern std::wstring targetDamperScale;
extern std::wstring targetSpringEnabled;



extern IDirectInputDevice8* matchedDevice;
extern LPDIRECTINPUT8 directInput;

// Include logging
void LogMessage(const std::wstring& msg);
void ListAvailableDevices();
void ShowAvailableDevicesOnConsole();

// === FFB Setup Functions ===
bool LoadFFBSettings(const std::wstring& filename);
bool InitializeDevice();