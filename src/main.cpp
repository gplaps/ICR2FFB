// FFB for ICR2
// I don't know what I am doing!
// Beta 0.8.8 Don't forget to update this down below


// File: main.cpp

/*
 * Copyright 2025 gplaps
 *
 * Licensed under the MIT License (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/MIT
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 */
 

// === Standard Library Includes ===
#include <iostream>
#include <iomanip>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <deque>
#include <fstream>
#include <unordered_set>
#include <vector>
#include <string>
#include <sstream>

// === Windows & DirectInput ===
#include <windows.h>
#include <initguid.h>
#include <dinput.h>

// === Project Includes ===
#include "constants.h"
#include "helpers.h"
#include "ffb_setup.h"
#include "telemetry_reader.h"
//#include "calculations/slip_angle.h"
//#include "calculations/lateral_load.h"
#include "calculations/vehicle_dynamics.h"
#include "forces/constant_force.h"
#include "forces/damper_effect.h"
#include "forces/spring_effect.h"

// Global log buffer
static std::mutex logMutex;
static std::deque<std::wstring> logLines;
const size_t maxLogLines = 1000;  // Show last 1000 log lines

// === Global Force Feedback Flags & States ===
// I have 3 effects right now which all get calculated separately
// I think probably all you need are these three to make good FFB

// Constant force is the most in depth
// Damper & Spring just use speed to do things
static bool enableRateLimit = false;
static bool enableConstantForce = false;
static bool enableWeightForce = false;
static bool enableDamperEffect = false;
static bool enableSpringEffect = false;

static bool constantStarted = false;
static bool damperStarted = false;
static bool springStarted = false;

// I think this is how we tell it these things are DirectInput stuff?
static IDirectInputEffect* constantForceEffect = NULL;
static IDirectInputEffect* damperEffect = NULL;
static IDirectInputEffect* springEffect = NULL;

static DIJOYSTATE2 js; // No idea what this is


// === Shared Telemetry Display Data ===
struct TelemetryDisplayData {
    // Raw telemetry
    double dlat;
    double dlong;
    double rotation_deg;
    double speed_mph;
    double steering_deg;
    double steering_raw;
    double long_force;

    // Tire loads
    double tireload_lf;
    double tireload_rf;
    double tireload_lr;
    double tireload_rr;

    // Tire magnitudes
    double tiremaglat_lf;
    double tiremaglat_rf;
    double tiremaglat_lr;
    double tiremaglat_rr;

    // Legacy calculated data
    int directionVal;
    double slipAngleDeg;
    double lateralG;
    double forceMagnitude;

    // Vehicle Dynamics calculated data
    double vd_lateralG;
    double vd_frontLeftForce_N;
    double vd_frontRightForce_N;
    int vd_directionVal;
    //double vd_yaw;
    double vd_slip;
    double vd_forceMagnitude;

    // Individual tire forces
    double vd_force_lf;
    double vd_force_rf;
    double vd_force_lr;
    double vd_force_rr;

    // Aggregate forces
    double vd_frontLateralForce;
    double vd_rearLateralForce;
    double vd_totalLateralForce;
    double vd_yawMoment;
};

// === Shared Globals ===
static std::mutex displayMutex;
static TelemetryDisplayData displayData;
static std::atomic<double> currentSpeed = {};
static std::atomic<bool> shouldExit = {};
extern int g_currentFFBForce;
int g_currentFFBForce = 0;

// Check Admin rights
static bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID administratorsGroup = NULL;

    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administratorsGroup)) {

        CheckTokenMembership(NULL, administratorsGroup, &isAdmin);
        FreeSid(administratorsGroup);
    }

    return isAdmin == TRUE;
}

static void RestartAsAdmin() {
    // Get the current executable path
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    // Use ShellExecuteW to restart with "runas" (admin prompt)
    HINSTANCE result = ShellExecuteW(
        NULL,                    // Parent window
        L"runas",               // Operation (request admin)
        exePath,                // Program to run
        NULL,                   // Command line arguments
        NULL,                   // Working directory
        SW_SHOWNORMAL           // Show window normally
    );

    // Check if the restart was successful
    if ((intptr_t)result > 32) {
        // Success - the new admin instance is starting, exit this one
        std::wcout << L"[INFO] Restarting with administrator privileges..." << std::endl;
        exit(0);
    }
    else {
        // Failed - user probably clicked "No" on UAC prompt
        std::wcout << L"[ERROR] Failed to restart as administrator." << std::endl;
        std::wcout << L"[ERROR] Please right-click the program and select 'Run as administrator'" << std::endl;
    }
}

// Console drawing stuff
// little function to help with display refreshing
// moves cursor to top without refreshing the screen

static void SetConsoleWindowSize() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        LogMessage(L"[ERROR] Failed to get console handle");
        return;
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hOut, &csbi)) {
        LogMessage(L"[ERROR] Failed to get console buffer info");
        return;
    }

    COORD bufferSize = { 120, 200 };           // scrollable size
    if (!SetConsoleScreenBufferSize(hOut, bufferSize)) {
        LogMessage(L"[WARNING] Failed to set console buffer size");
    }

    SMALL_RECT windowSize = { 0, 0, 119, 39 };  // window size (note: 119, not 120)
    if (!SetConsoleWindowInfo(hOut, TRUE, &windowSize)) {
        LogMessage(L"[WARNING] Failed to set console window size");
    }
    else {
        LogMessage(L"[INFO] Console window size set successfully");
    }
}

// Prevent lockup if window is clicked
static void DisableConsoleQuickEdit() {
    HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (hInput == INVALID_HANDLE_VALUE) {
        LogMessage(L"[ERROR] Failed to get input handle");
        return;
    }

    DWORD mode;
    if (!GetConsoleMode(hInput, &mode)) { 
        LogMessage(L"[ERROR] Failed to get console mode");
        return;
    }

 
    mode &= static_cast<DWORD>(~(ENABLE_QUICK_EDIT_MODE | ENABLE_INSERT_MODE));
    mode |= ENABLE_EXTENDED_FLAGS;

    if (!SetConsoleMode(hInput, mode)) { 
        LogMessage(L"[ERROR] Failed to set console mode");
    }
    else {
        LogMessage(L"[INFO] Console Quick Edit Mode disabled");
    }
}

static void MoveCursorToTop() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD topLeft = { 0, 0 };
    SetConsoleCursorPosition(hConsole, topLeft);
}

static void MoveCursorToLine(short lineNumber) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { 0, lineNumber };
    SetConsoleCursorPosition(hConsole, pos);
}

static void HideConsoleCursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) {
        LogMessage(L"[ERROR] Failed to get console handle for cursor");
        return;
    }

    CONSOLE_CURSOR_INFO cursorInfo;
    if (!GetConsoleCursorInfo(hOut, &cursorInfo)) {
        LogMessage(L"[ERROR] Failed to get cursor info");
        return;
    }

    cursorInfo.bVisible = FALSE;
    if (!SetConsoleCursorInfo(hOut, &cursorInfo)) {
        LogMessage(L"[ERROR] Failed to hide cursor");
    }
    else {
        LogMessage(L"[INFO] Cursor hidden successfully");
    }
}

// New display

static void DisplayTelemetry(const TelemetryDisplayData& displayDataIn, double masterForceValue) {
    // Move cursor to top and set up formatting
    MoveCursorToTop();
    std::cout << std::fixed << std::setprecision(2);
    std::wcout << std::fixed << std::setprecision(2);  // Also set for wide cout

    // Define console width
    const unsigned int CONSOLE_WIDTH = 80;

    // Helper lambda to pad lines
    auto padLine = [&](const std::wstring& text) {
        std::wstring padded = text;
        if (padded.length() < CONSOLE_WIDTH) {
            padded.append(CONSOLE_WIDTH - padded.length(), L' ');
        }
        else if (padded.length() > CONSOLE_WIDTH) {
            padded = padded.substr(0, CONSOLE_WIDTH);  // Truncate if too long
        }
        return padded;
        };

    // Header section
    std::wcout << padLine(L"ICR2 FFB Program Version 0.8.8 BETA") << L"\n";
    std::wcout << padLine(L"USE AT YOUR OWN RISK") << L"\n";
    std::wcout << padLine(L"Connected Device: " + targetDeviceName) << L"\n";

    std::wostringstream ss;
    ss << std::fixed << std::setprecision(2);  // Set formatting for stringstream too
    ss << L"Master Force Scale: " << masterForceValue << L"%";
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";  // Empty line

    // Raw data section
    std::wcout << padLine(L"      == Raw Data ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L""); ss.clear();
    ss << L"dLat: " << std::setw(10) << displayDataIn.dlat << L"   dLong: " << std::setw(10) << displayDataIn.dlong;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Centerline Rotation: " << std::setw(8) << displayDataIn.rotation_deg << L" deg";
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Speed: " << std::setw(8) << displayDataIn.speed_mph << L" mph";
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Steering Raw: " << std::setw(10) << displayDataIn.steering_raw;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Steering Lock Degree: " << std::setw(8) << displayDataIn.steering_deg;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    // Tire loads section
    std::wcout << padLine(L"      == Tire Loads ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";
    std::wcout << padLine(L"Front Left      Front Right") << L"\n";

    ss.str(L""); ss.clear();
    ss << std::setw(10) << displayDataIn.vd_frontLeftForce_N << L"           " << std::setw(10) << displayDataIn.vd_frontRightForce_N;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L""); ss.clear();
    ss << std::setw(10) << static_cast<int16_t>(displayDataIn.tiremaglat_lf) << L"           " << std::setw(10) << static_cast<int16_t>(displayDataIn.tiremaglat_rf);
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    std::wcout << padLine(L"Rear Left       Rear Right") << L"\n";
    //ss.str(L""); ss.clear();
    //ss << std::setw(10) << displayDataIn.tireload_lr << L"           " << std::setw(10) << displayDataIn.tireload_rr;
    //std::wcout << padLine(ss.str()) << L"\n";
    //std::wcout << padLine(L"") << L"\n";

    ss.str(L""); ss.clear();
    ss << std::setw(10) << static_cast<int16_t>(displayDataIn.tiremaglat_lr) << L"           " << std::setw(10) << static_cast<int16_t>(displayDataIn.tiremaglat_rr);
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    // Vehicle Dynamics section
    std::wcout << padLine(L"      == Vehicle Dynamics ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Lateral G: " << std::setw(8) << displayDataIn.vd_lateralG << L" G";
    std::wcout << padLine(ss.str()) << L"\n";

    //ss.str(L""); ss.clear();
    //ss << L"Yaw Rate: " << std::setw(8) << displayDataIn.vd_yaw << L" deg/s�";
    //std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Longi Force: " << std::setw(8) << displayDataIn.long_force << L"";
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Direction Value: " << displayDataIn.vd_directionVal;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Force Magnitude: " << g_currentFFBForce;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    /*
    // Tire Forces
    std::wcout << padLine(L"      == Decoded Tire Forces ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";
    std::wcout << padLine(L"Front Left      Front Right") << L"\n";

    ss.str(L""); ss.clear();
    ss << std::setw(10) << displayDataIn.vd_force_lf << L"           " << std::setw(10) << displayDataIn.vd_force_rf;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    std::wcout << padLine(L"Rear Left       Rear Right") << L"\n";
    ss.str(L""); ss.clear();
    ss << std::setw(10) << displayDataIn.vd_force_lr << L"           " << std::setw(10) << displayDataIn.vd_force_rr;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Front Total: " << std::setw(8) << displayDataIn.vd_frontLateralForce << L"   Rear Total: " << std::setw(8) << displayDataIn.vd_rearLateralForce;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Total Force: " << std::setw(8) << displayDataIn.vd_totalLateralForce << L"   Yaw Moment: " << std::setw(8) << displayDataIn.vd_yawMoment;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";
    */

    /*
    // Legacy calculated data section
    std::wcout << padLine(L"      == Legacy Calculated Data ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Direction Value: " << displayDataIn.directionVal;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Slip: " << displayDataIn.slipAngleDeg;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Lateral G: " << displayDataIn.lateralG << L" G";
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Force Magnitude: " << static_cast<int>(displayDataIn.forceMagnitude * (double)DEFAULT_DINPUT_GAIN);
    std::wcout << padLine(ss.str()) << L"\n";

    */
    std::wcout << padLine(L"----------------------------------------") << L"\n";
    std::wcout << padLine(L"Log:") << L"\n";
}

// Logging stuff - Keeps messages for future debugging!
// Write to log.txt
void LogMessage(const std::wstring& msg) {
    std::lock_guard<std::mutex> lock(logMutex);

    // Add to in-memory deque for optional UI display (if needed)
    logLines.push_back(msg);
    while (logLines.size() > maxLogLines)
        logLines.pop_front();

    // Append to log.txt
    std::wofstream logFile("log.txt", std::ios::app);
    if (logFile.is_open()) {
        logFile << msg << std::endl;
    }
}

// === Force Effect Creators ===
static void CreateConstantForceEffect(LPDIRECTINPUTDEVICE8 device) {
    if (!device) return;

    DICONSTANTFORCE cf = { 0 };

    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwGain = DEFAULT_DINPUT_GAIN;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.cAxes = 1;
    DWORD axes[1] = { DIJOFS_X };
    LONG dir[1] = { 0 };
    eff.rgdwAxes = axes;
    eff.rglDirection = dir;
    eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;

    DIPROPRANGE diprg = {};
    diprg.diph.dwSize = sizeof(DIPROPRANGE);
    diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    diprg.diph.dwHow = DIPH_BYOFFSET;
    diprg.diph.dwObj = DIJOFS_X;
    diprg.lMin = -static_cast<LONG>(DEFAULT_DINPUT_GAIN);
    diprg.lMax = DEFAULT_DINPUT_GAIN;
    matchedDevice->SetProperty(DIPROP_RANGE, &diprg.diph);

    HRESULT hr = device->CreateEffect(GUID_ConstantForce, &eff, &constantForceEffect, NULL);
    if (FAILED(hr)) {
        LogMessage(L"[ERROR] Failed to create constant force effect. HRESULT: 0x" + std::to_wstring(hr));
    }
    else {
        LogMessage(L"[INFO] Initial constant force created");
    }
}

static void CreateDamperEffect(IDirectInputDevice8* device) {
    if (!device) return;

    DICONDITION condition = {};
    condition.lOffset = 0;
    condition.lPositiveCoefficient = 8000;
    condition.lNegativeCoefficient = 8000;
    condition.dwPositiveSaturation = DEFAULT_DINPUT_GAIN;
    condition.dwNegativeSaturation = DEFAULT_DINPUT_GAIN;
    condition.lDeadBand = 0;

    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwGain = DEFAULT_DINPUT_GAIN;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.cAxes = 1;
    DWORD axes[1] = { DIJOFS_X };
    LONG dir[1] = { 0 };
    eff.rgdwAxes = axes;
    eff.rglDirection = dir;
    eff.cbTypeSpecificParams = sizeof(DICONDITION);
    eff.lpvTypeSpecificParams = &condition;

    HRESULT hr = device->CreateEffect(GUID_Damper, &eff, &damperEffect, NULL);
    if (FAILED(hr) || !damperEffect) {
        LogMessage(L"[ERROR] Failed to create damper effect. HRESULT: 0x" + std::to_wstring(hr));
    }
    else {
        LogMessage(L"[INFO] Initial damper effect created");
    }
}

static void CreateSpringEffect(IDirectInputDevice8* device) {
    if (!device) return;

    DICONDITION condition = {};
    condition.lOffset = 0;
    condition.lPositiveCoefficient = 8000;
    condition.lNegativeCoefficient = 8000;
    condition.dwPositiveSaturation = DEFAULT_DINPUT_GAIN;
    condition.dwNegativeSaturation = DEFAULT_DINPUT_GAIN;
    condition.lDeadBand = 0;

    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwGain = DEFAULT_DINPUT_GAIN;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.cAxes = 1;
    DWORD axes[1] = { DIJOFS_X };
    LONG dir[1] = { 0 };
    eff.rgdwAxes = axes;
    eff.rglDirection = dir;
    eff.cbTypeSpecificParams = sizeof(DICONDITION);
    eff.lpvTypeSpecificParams = &condition;

    HRESULT hr = device->CreateEffect(GUID_Spring, &eff, &springEffect, NULL);
    if (FAILED(hr) || !springEffect) {
        LogMessage(L"[ERROR] Failed to create spring effect. HRESULT: 0x" + std::to_wstring(hr));
    }
    else {
        LogMessage(L"[INFO] Initial spring effect created");
    }
}

// Loop which kicks stuff off and coordinates everything!
static void ProcessLoop() {
   
    //Get some data from RawTelemetry -> not 100% sure what this does
    RawTelemetry current{};
    RawTelemetry previousLat{};
    RawTelemetry previousSlip{};
    RawTelemetry previousVD{};
    bool firstReadingLat = true;
    bool firstReadingSlip = true;
    bool firstReadingVD = true;

    double previousDlong = 0.0; (void)previousDlong; // currently unused
    int noMovementFrames = 0; (void) noMovementFrames; // currently unused
    const int movementThreshold = 3;  // number of frames to consider "stopped"
    (void)movementThreshold; // currently unused
    bool effectPaused = false; (void)effectPaused; // currently unused

    //Added for feedback skipping if stopped
    RawTelemetry previousPos{};
    bool firstPos = true;

    while (!shouldExit) {

        // Check to see if Telemetry is coming in, but if not then wait for it!
        if (!ReadTelemetryData(current)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }

        if (firstPos) { previousPos = current; firstPos = false; }


        // Start damper/spring effects once telemetry is valid
        // Probably need to also figure out how to stop these when the game pauses
        // Also need to maybe fade in and out the effects when waking/sleeping
        if (!damperStarted && damperEffect && enableDamperEffect) {
            damperEffect->Start(1, 0);
            damperStarted = true;
            LogMessage(L"[INFO] Damper effect started");
        }

        if (!springStarted && springEffect && enableSpringEffect) {
            springEffect->Start(1, 0);
            springStarted = true;
            LogMessage(L"[INFO] Spring effect started");
        }

        // Master force scale -> Keeping Hands Safe
        double masterForceValue = std::stod(targetForceSetting);
        double masterForceScale = std::clamp(masterForceValue / 100.0, 0.0, 1.0);

        double deadzoneForceValue = std::stod(targetDeadzoneSetting);
        double deadzoneForceScale = std::clamp(deadzoneForceValue / 100.0, 0.0, 1.0);

        double constantForceValue = std::stod(targetConstantScale);
        double constantForceScale = std::clamp(constantForceValue / 100.0, 0.0, 1.0);

        double weightForceValue = std::stod(targetWeightScale);
        double weightForceScale = std::clamp(weightForceValue / 100.0, 0.0, 1.0);

        double damperForceValue = std::stod(targetDamperScale);
        double damperForceScale = std::clamp(damperForceValue / 100.0, 0.0, 1.0);

        // Update Effects
        if (damperEffect && enableDamperEffect)
            UpdateDamperEffect(current.speed_mph, damperEffect, masterForceScale, damperForceScale);

        if (springEffect && enableSpringEffect)
            UpdateSpringEffect(springEffect, masterForceScale);
            
        // Do Force calculations based on raw data
        // Right now its "Slip" and "Lateral Load"
        CalculatedSlip slip{};
        if (CalculateSlipAngle(current, previousSlip, firstReadingSlip, slip)) {
            displayData.slipAngleDeg = slip.slipAngle;
        }

        CalculatedVehicleDynamics vehicleDynamics{};
        bool vehicleDynamicsValid = CalculateVehicleDynamics(current, previousVD, firstReadingVD, vehicleDynamics);

        CalculatedLateralLoad load{};
        if (CalculateLateralLoad(current, previousLat, firstReadingLat, slip, load)) {
            // Poll input state
            if (FAILED(matchedDevice->Poll())) {
                matchedDevice->Acquire();
                matchedDevice->Poll();
            }
            matchedDevice->GetDeviceState(sizeof(DIJOYSTATE2), &js);

            // Start constant force once telemetry is valid 
            if (enableConstantForce && constantForceEffect) {
                if (!constantStarted) {
                    constantForceEffect->Start(1, 0);
                    constantStarted = true;
                    LogMessage(L"[INFO] Constant force started");
                }

                //This is what will add the "Constant Force" effect if all the calculations work. 
                // Probably could smooth all this out
                ApplyConstantForceEffect(current, load, slip, 
                    vehicleDynamics, current.speed_mph, current.steering_deg, constantForceEffect, enableWeightForce, enableRateLimit, 
                    masterForceScale, deadzoneForceScale,
                    constantForceScale, weightForceScale);
                previousPos = current;

            }


/*
            // Auto-pause force if not moving
            // I think this is broken or I could detect pause in a better way
            // Maybe DLONG not moving?
            bool isStationary = std::abs(current.dlong - previousDlong) < 0.01;
            if (isStationary) {
                noMovementFrames++;
                if (noMovementFrames >= movementThreshold && !effectPaused) {
                    //add damper and spring?
                    constantForceEffect->Stop();
                    effectPaused = true;
                    LogMessage(L"[INFO] FFB paused due to no movement");
                }
            }
            else {
                noMovementFrames = 0;
              //  if (effectPaused) {
                    //add damper and spring?
              //      constantForceEffect->Start(1, 0);
              //      effectPaused = false;
              //      std::wcout << L"FFB resumed\n";
              //  }
              // Added Alpha v0.6 to prevent issues with Moza?
                if (effectPaused) {
                    HRESULT startHr = constantForceEffect->Start(1, 0);
                    if (FAILED(startHr)) {
                        LogMessage(L"[ERROR] Failed to restart constant force: 0x" + std::to_wstring(startHr));
                    }
                    else {
                        effectPaused = false;
                        LogMessage(L"[INFO] FFB resumed");
                    }
                }   
            }

*/

            //Setting variables for next update
            previousDlong = current.dlong;
            currentSpeed = current.speed_mph;


            // Update telemetry for display
            {
                std::lock_guard<std::mutex> lock(displayMutex);

                // Basic telemetry
                displayData.dlat = current.dlat;
                displayData.dlong = current.dlong;
                displayData.rotation_deg = current.rotation_deg;
                displayData.speed_mph = current.speed_mph;
                displayData.steering_deg = current.steering_deg;
                displayData.steering_raw = current.steering_raw;
                displayData.long_force = current.long_force;


                // Tire loads
                displayData.tireload_lf = current.tireload_lf;
                displayData.tireload_rf = current.tireload_rf;
                displayData.tireload_lr = current.tireload_lr;
                displayData.tireload_rr = current.tireload_rr;

                // Tire magnitudes
                displayData.tiremaglat_lf = current.tiremaglat_lf;
                displayData.tiremaglat_rf = current.tiremaglat_rf;
                displayData.tiremaglat_lr = current.tiremaglat_lr;
                displayData.tiremaglat_rr = current.tiremaglat_rr;

                // Legacy calculated data
                displayData.directionVal = slip.directionVal;
                displayData.slipAngleDeg = slip.slipAngle;
                displayData.lateralG = load.lateralG;
                displayData.forceMagnitude = load.forceMagnitude;

                // NEW: Vehicle dynamics data (only update if calculation was successful)
                if (vehicleDynamicsValid) {
                    displayData.vd_lateralG = vehicleDynamics.lateralG;
                    displayData.vd_directionVal = vehicleDynamics.directionVal;
                    displayData.vd_frontLeftForce_N = vehicleDynamics.frontLeftForce_N;
                    displayData.vd_frontRightForce_N = vehicleDynamics.frontRightForce_N;
                    //displayData.vd_yaw = vehicleDynamics.yaw;
                    displayData.vd_slip = vehicleDynamics.slip;
                    displayData.vd_forceMagnitude = vehicleDynamics.forceMagnitude;

                    // Individual tire forces
                    displayData.vd_force_lf = vehicleDynamics.force_lf;
                    displayData.vd_force_rf = vehicleDynamics.force_rf;
                    displayData.vd_force_lr = vehicleDynamics.force_lr;
                    displayData.vd_force_rr = vehicleDynamics.force_rr;

                    // Aggregate forces
                    displayData.vd_frontLateralForce = vehicleDynamics.frontLateralForce;
                    displayData.vd_rearLateralForce = vehicleDynamics.rearLateralForce;
                    displayData.vd_totalLateralForce = vehicleDynamics.totalLateralForce;
                    displayData.vd_yawMoment = vehicleDynamics.yawMoment;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

static BOOL WINAPI ConsoleHandler(DWORD CEvent) noexcept
{
    switch(CEvent)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        shouldExit = true;
        break;
    default: break;
    }
    return TRUE;
}

// Where it all happens
int main() {
    
    if (!IsRunningAsAdmin()) {
        std::wcout << L"===============================================" << std::endl;
        std::wcout << L"    ICR2 FFB Program - Admin Rights Required" << std::endl;
        std::wcout << L"===============================================" << std::endl;
        std::wcout << L"" << std::endl;
        std::wcout << L"This program requires administrator privileges to:" << std::endl;
        std::wcout << L"  - Access DirectInput force feedback devices" << std::endl;
        std::wcout << L"  - Control console display properly" << std::endl;
        std::wcout << L"  - Ensure reliable wheel communication" << std::endl;
        std::wcout << L"" << std::endl;
        std::wcout << L"Would you like to restart as administrator? (y/n): ";

        wchar_t response;
        std::wcin >> response;

        if (response == L'y' || response == L'Y') {
            RestartAsAdmin();
            // If we get here, the restart failed
            std::wcout << L"Press any key to exit..." << std::endl;
            std::cin.get();
            return 1;
        }
        else {
            std::wcout << L"" << std::endl;
            std::wcout << L"Cannot continue without administrator privileges." << std::endl;
            std::wcout << L"Please restart the program as administrator." << std::endl;
            std::wcout << L"Press any key to exit..." << std::endl;
            std::wcin.ignore();
            std::wcin.get();
            return 1;
        }
    }

    SetConsoleWindowSize();
    HideConsoleCursor();
    DisableConsoleQuickEdit();
    if (SetConsoleCtrlHandler(
        static_cast<PHANDLER_ROUTINE>(ConsoleHandler),TRUE)==FALSE)
    {
        LogMessage(L"[ERROR] Unable to install keyboard ctrl handler");
        return -1;
    }

    //clear last log
    std::wofstream clearLog("log.txt", std::ios::trunc);


    // Load FFB configuration file "ffb.ini"
    if (!LoadFFBSettings(L"ffb.ini")) {
        LogMessage(L"[ERROR] Failed to load FFB settings from ffb.ini");
        LogMessage(L"[ERROR] Make sure ffb.ini exists in current working directory and has proper format");

        // SHOW ERROR ON CONSOLE immediately
        std::wcout << L"[ERROR] Failed to load FFB settings from ffb.ini" << std::endl;
        std::wcout << L"[ERROR] Make sure ffb.ini exists in current working directory and has proper format" << std::endl;
        std::wcout << L"Press any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    LogMessage(L"[INFO] Successfully loaded FFB settings");
    LogMessage(L"[INFO] Target device: " + targetDeviceName);

    // Initialize DirectInput device
    // I don't really know how this works yet. It enabled "exclusive" mode on the device and calls it a "type 2" joystick.. OK!
    if (!InitializeDevice()) {
        LogMessage(L"[ERROR] Failed to initialize DirectInput or find device: " + targetDeviceName);
        LogMessage(L"[ERROR] Available devices:");

        // List available devices to help user
        ListAvailableDevices();

        LogMessage(L"[ERROR] Check your ffb.ini file - device name must match exactly");

        // SHOW ERROR ON CONSOLE immediately
        std::wcout << L"[ERROR] Could not find controller: " << targetDeviceName << std::endl;
        std::wcout << L"[ERROR] Available devices:" << std::endl;

        // Show available devices on console too
        ShowAvailableDevicesOnConsole();

        std::wcout << L"[ERROR] Check your ffb.ini file - device name must match exactly" << std::endl;
        std::wcout << L"Press any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    LogMessage(L"[INFO] Device found and initialized successfully");


    // ONLY configure device if it was found successfully
    HRESULT hr;
    hr = matchedDevice->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(hr)) {
        LogMessage(L"[ERROR] Failed to set data format: 0x" + std::to_wstring(hr));
        
        std::wcout << L"[ERROR] Failed to set data format: 0x" << std::hex << hr << std::endl;
        std::wcout << L"Press any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    hr = matchedDevice->SetCooperativeLevel(GetConsoleWindow(), DISCL_BACKGROUND | DISCL_EXCLUSIVE);
    if (FAILED(hr)) {
        LogMessage(L"[ERROR] Failed to set cooperative level: 0x" + std::to_wstring(hr));
        LogMessage(L"[ERROR] Another application may be using the device exclusively");
        
        std::wcout << L"[ERROR] Failed to set cooperative level: 0x" << std::hex << hr << std::endl;
        std::wcout << L"[ERROR] Another application may be using the device exclusively" << std::endl;
        std::wcout << L"Press any key to exit..." << std::endl;
        std::cin.get();
        return 1;
    }

    hr = matchedDevice->Acquire();
    if (FAILED(hr)) {
        LogMessage(L"[WARNING] Initial acquire failed: 0x" + std::to_wstring(hr) + L" (this is often normal)");
    }
    else {
        LogMessage(L"[INFO] Device acquired successfully");
    }

    // Parse FFB effect toggles from config <- should all ffb types be enabled? Allows user to select if they dont like damper for instance
    // Would be nice to add a % per effect in the future
    enableRateLimit = ToLower(targetWeightEnabled) == L"true";
    enableConstantForce = ToLower(targetConstantEnabled) == L"true";
    enableWeightForce = ToLower(targetWeightEnabled) == L"true";
    enableDamperEffect = ToLower(targetDamperEnabled) == L"true";
    enableSpringEffect = ToLower(targetSpringEnabled) == L"true";

    // Create FFB effects as needed
    if (enableConstantForce) CreateConstantForceEffect(matchedDevice);
    if (enableDamperEffect)  CreateDamperEffect(matchedDevice);
    if (enableSpringEffect)  CreateSpringEffect(matchedDevice);

    // This is to control the max % for any of the FFB effects as specified in the ffb.ini
    // Prevents broken wrists (hopefully)
    double masterForceValue = std::stod(targetForceSetting); (void)masterForceValue; // currently unused
    double constantForceValue = std::stod(targetConstantScale); (void)constantForceValue; // currently unused
    double weightForceValue = std::stod(targetWeightScale); (void)weightForceValue; // currently unused
    double damperForceValue = std::stod(targetDamperScale); (void)damperForceValue; // currently unused

    // Start telemetry processing!
    std::thread processThread(ProcessLoop);
    processThread.detach();

    // Now that we're doing everything we can display stuff!
    // Main Display Loop - Set to 200ms? Probably fine
    // Flickers a lot right now but perhaps moving to a GUI will solve that eventually
    while (!shouldExit) {

        // make sure we stay at most recent display update
        MoveCursorToLine(0);

        //Trigger display
        {
            std::lock_guard<std::mutex> lock(displayMutex);
            DisplayTelemetry(displayData, masterForceValue);
        }

        //Print log data
        {
            std::lock_guard<std::mutex> lock(logMutex);
            unsigned int maxDisplayLines = 1; //how many lines to display
            std::vector<std::wstring> recentUniqueLines;
            std::unordered_set<std::wstring> seen;

            // Go backward to find most recent unique messages
            for (auto it = logLines.rbegin(); it != logLines.rend() && recentUniqueLines.size() < maxDisplayLines; ++it) {
                if (seen.insert(*it).second) {
                    recentUniqueLines.push_back(*it);
                }
            }

            // Reverse to show most recent at bottom
            std::reverse(recentUniqueLines.begin(), recentUniqueLines.end());
            
            for (const auto& line : recentUniqueLines) {
                std::wstring padded = line;
                padded.resize(80, L' ');  // pad to 80 characters to clear old line leftovers
                std::wcout << padded << L"\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}
