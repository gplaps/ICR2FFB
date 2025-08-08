// FFB for ICR2
// I don't know what I am doing!
// Beta 0.4 Don't forget to update this down below


// File: main.cpp
 

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
#include <dinput.h>

// === Project Includes ===
#include "ffb_setup.h"
#include "telemetry_reader.h"
#include "calculations/slip_angle.h"
#include "calculations/lateral_load.h"
#include "forces/constant_force.h"
#include "forces/damper_effect.h"
#include "forces/spring_effect.h"

// Global log buffer
std::mutex logMutex;
std::deque<std::wstring> logLines;
const size_t maxLogLines = 1000;  // Show last 1000 log lines

// === Global Force Feedback Flags & States ===
// I have 3 effects right now which all get calculated separately
// I think probably all you need are these three to make good FFB

// Constant force is the most in depth
// Damper & Spring just use speed to do things
bool enableConstantForce = false;
bool enableDamperEffect = false;
bool enableSpringEffect = false;

bool constantStarted = false;
bool damperStarted = false;
bool springStarted = false;

// I think this is how we tell it these things are DirectInput stuff?
IDirectInputEffect* constantForceEffect = nullptr;
IDirectInputEffect* damperEffect = nullptr;
IDirectInputEffect* springEffect = nullptr;

DIJOYSTATE2 js; // No idea what this is


// === Shared Telemetry Display Data ===
struct TelemetryDisplayData {
    // Raw telemetry
    double dlat = 0.0;
    double dlong = 0.0;
    double rotation_deg = 0.0;
    double speed_mph = 0.0;
    double steering_deg = 0.0;
    double steering_raw = 0.0;
    double tireload_lf = 0.0;
    double tireload_rf = 0.0;
    double tireload_lr = 0.0;
    double tireload_rr = 0.0;
    double tiremaglat_lf = 0.0;
    double tiremaglat_rf = 0.0;
    double tiremaglat_lr = 0.0;
    double tiremaglat_rr = 0.0;

    // Slip values
    double slipNorm_lf = 0.0;
    double slipNorm_rf = 0.0;
    double slipNorm_lr = 0.0;
    double slipNorm_rr = 0.0;
    double slipMag_lf = 0.0;
    double slipMag_rf = 0.0;
    double slipMag_lr = 0.0;
    double slipMag_rr = 0.0;

    // Calculated forces
    double slipAngleDeg = 0.0;
    double lateralG = 0.0;
    int forceMagnitude = 0;
    int directionVal = 0;
};

// === Shared Globals ===
std::mutex displayMutex;
TelemetryDisplayData displayData;
std::atomic<double> currentSpeed = 0.0;

// Check Admin rights
bool IsRunningAsAdmin() {
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

void RestartAsAdmin() {
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

void SetConsoleWindowSize() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);

    SMALL_RECT windowSize = { 0, 0, 119, 39 };  // window size
    COORD bufferSize = { 120, 200 };           // scrollable size

    SetConsoleScreenBufferSize(hOut, bufferSize);
    SetConsoleWindowInfo(hOut, TRUE, &windowSize);
}

void MoveCursorToTop() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD topLeft = { 0, 0 };
    SetConsoleCursorPosition(hConsole, topLeft);
}

void MoveCursorToLine(short lineNumber) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { 0, lineNumber };
    SetConsoleCursorPosition(hConsole, pos);
}

void HideConsoleCursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(hOut, &cursorInfo);
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hOut, &cursorInfo);
}

// New display

void DisplayTelemetry(const TelemetryDisplayData& displayData, double masterForceValue) {
    // Move cursor to top and set up formatting
    MoveCursorToTop();
    std::cout << std::fixed << std::setprecision(2);
    std::wcout << std::fixed << std::setprecision(2);  // Also set for wide cout

    // Define console width
    const int CONSOLE_WIDTH = 80;

    // Helper lambda to pad lines
    auto padLine = [CONSOLE_WIDTH](const std::wstring& text) {
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
    std::wcout << padLine(L"ICR2 FFB Program Version 0.4 BETA") << L"\n";
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
    ss << L"dLat: " << std::setw(10) << displayData.dlat << L"   dLong: " << std::setw(10) << displayData.dlong;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Centerline Rotation: " << std::setw(8) << displayData.rotation_deg << L" deg";
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Speed: " << std::setw(8) << displayData.speed_mph << L" mph";
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Steering Raw: " << std::setw(10) << displayData.steering_raw;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Steering Lock Degree: " << std::setw(8) << displayData.steering_deg;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    // Tire loads section
    std::wcout << padLine(L"      == Tire Loads ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";
    std::wcout << padLine(L"Front Left      Front Right") << L"\n";

    ss.str(L""); ss.clear();
    ss << std::setw(10) << displayData.tireload_lf << L"           " << std::setw(10) << displayData.tireload_rf;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    std::wcout << padLine(L"Rear Left       Rear Right") << L"\n";
    ss.str(L""); ss.clear();
    ss << std::setw(10) << displayData.tireload_lr << L"           " << std::setw(10) << displayData.tireload_rr;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    // Calculated data section
    std::wcout << padLine(L"      == Calculated Data (probably wrong) ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Direction Value: " << displayData.directionVal;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Slip: " << displayData.slipAngleDeg;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Lateral G: " << displayData.lateralG << L" G";
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Force Magnitude: " << displayData.forceMagnitude;
    std::wcout << padLine(ss.str()) << L"\n";

    std::wcout << padLine(L"----------------------------------------") << L"\n";
    std::wcout << padLine(L"Log:") << L"\n";
}

// Logging stuff - Keeps messages for future debugging!
// Write to log.txt
void LogMessage(const std::wstring& msg) {
    std::lock_guard<std::mutex> lock(logMutex);

    // Add to in-memory deque for optional UI display (if needed)
    logLines.push_back(msg);
    if (logLines.size() > maxLogLines)
        logLines.pop_front();

    // Append to log.txt
    std::wofstream logFile("log.txt", std::ios::app);
    if (logFile.is_open()) {
        logFile << msg << std::endl;
    }
}

// === Force Effect Creators ===
void CreateConstantForceEffect(LPDIRECTINPUTDEVICE8 device) {
    if (!device) return;

    DICONSTANTFORCE cf = { 0 };

    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwGain = 10000;
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
    diprg.lMin = -10000;
    diprg.lMax = 10000;
    matchedDevice->SetProperty(DIPROP_RANGE, &diprg.diph);

    HRESULT hr = device->CreateEffect(GUID_ConstantForce, &eff, &constantForceEffect, nullptr);
    if (FAILED(hr)) {
        LogMessage(L"[ERROR] Failed to create constant force effect. HRESULT: 0x" + std::to_wstring(hr));
    }
    else {
        LogMessage(L"[INFO] Initial constant force created");
    }
}

void CreateDamperEffect(IDirectInputDevice8* device) {
    if (!device) return;

    DICONDITION condition = {};
    condition.lOffset = 0;
    condition.lPositiveCoefficient = 8000;
    condition.lNegativeCoefficient = 8000;
    condition.dwPositiveSaturation = 10000;
    condition.dwNegativeSaturation = 10000;
    condition.lDeadBand = 0;

    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwGain = 10000;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.cAxes = 1;
    DWORD axes[1] = { DIJOFS_X };
    LONG dir[1] = { 0 };
    eff.rgdwAxes = axes;
    eff.rglDirection = dir;
    eff.cbTypeSpecificParams = sizeof(DICONDITION);
    eff.lpvTypeSpecificParams = &condition;

    HRESULT hr = device->CreateEffect(GUID_Damper, &eff, &damperEffect, nullptr);
    if (FAILED(hr) || !damperEffect) {
        LogMessage(L"[ERROR] Failed to create damper effect. HRESULT: 0x" + std::to_wstring(hr));
    }
    else {
        LogMessage(L"[INFO] Initial damper effect created");
    }
}

void CreateSpringEffect(IDirectInputDevice8* device) {
    if (!device) return;

    DICONDITION condition = {};
    condition.lOffset = 0;
    condition.lPositiveCoefficient = 8000;
    condition.lNegativeCoefficient = 8000;
    condition.dwPositiveSaturation = 10000;
    condition.dwNegativeSaturation = 10000;
    condition.lDeadBand = 0;

    DIEFFECT eff = {};
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwGain = 10000;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.cAxes = 1;
    DWORD axes[1] = { DIJOFS_X };
    LONG dir[1] = { 0 };
    eff.rgdwAxes = axes;
    eff.rglDirection = dir;
    eff.cbTypeSpecificParams = sizeof(DICONDITION);
    eff.lpvTypeSpecificParams = &condition;

    HRESULT hr = device->CreateEffect(GUID_Spring, &eff, &springEffect, nullptr);
    if (FAILED(hr) || !springEffect) {
        LogMessage(L"[ERROR] Failed to create spring effect. HRESULT: 0x" + std::to_wstring(hr));
    }
    else {
        LogMessage(L"[INFO] Initial spring effect created");
    }
}

// Loop which kicks stuff off and coordinates everything!
void ProcessLoop() {
   
    //Get some data from RawTelemetry -> not 100% sure what this does
    RawTelemetry current{};
    RawTelemetry previousLat{};
    RawTelemetry previousSlip{};
    bool firstReadingLat = true;
    bool firstReadingSlip = true;

    double previousDlong = 0.0;
    int noMovementFrames = 0;
    const int movementThreshold = 3;  // number of frames to consider "stopped"
    bool effectPaused = false;

    //Added for feedback skipping if stopped
    RawTelemetry previousPos{};
    bool firstPos = true;

    while (true) {

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

        // Update Effects
        if (damperEffect && enableDamperEffect)
            UpdateDamperEffect(current.speed_mph, damperEffect, masterForceScale);

        if (springEffect && enableSpringEffect)
            UpdateSpringEffect(springEffect, masterForceScale);
            
        // Do Force calculations based on raw data
        // Right now its "Slip" and "Lateral Load"
        CalculatedSlip slip{};
        if (CalculateSlipAngle(current, previousSlip, firstReadingSlip, slip)) {
            displayData.slipAngleDeg = slip.slipAngle;
        }

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
                ApplyConstantForceEffect(current, previousPos, load, slip,
                    current.speed_mph, constantForceEffect, masterForceScale);
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

                displayData = {
                    current.dlat,
                    current.dlong,
                    current.rotation_deg,
                    current.speed_mph,
                    current.steering_raw,
                    current.steering_deg,
                    current.tireload_lf,
                    current.tireload_rf,
                    current.tireload_lr,
                    current.tireload_rr,
                    current.tiremaglat_lf,
                    current.tiremaglat_rf,
                    current.tiremaglat_lr,
                    current.tiremaglat_rr,
                    slip.slipAngle,
                    slip.slipNorm_lf, slip.slipNorm_rf, slip.slipNorm_lr, slip.slipNorm_rr,
                    slip.slipMag_lf, slip.slipMag_rf, slip.slipMag_lr, slip.slipMag_rr,
                    load.lateralG,
                    load.forceMagnitude,
                    load.directionVal
                };
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
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

    //clear last log
    std::wofstream clearLog("log.txt", std::ios::trunc);


    // Load FFB configuration file "ffb.ini"
    if (!LoadFFBSettings(L"ffb.ini")) {
        LogMessage(L"[ERROR] Failed to load FFB settings from ffb.ini");
        LogMessage(L"[ERROR] Make sure ffb.ini exists and has proper format");

        // SHOW ERROR ON CONSOLE immediately
        std::wcout << L"[ERROR] Failed to load FFB settings from ffb.ini" << std::endl;
        std::wcout << L"[ERROR] Make sure ffb.ini exists and has proper format" << std::endl;
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
    enableConstantForce = (targetConstantEnabled == L"true" || targetConstantEnabled == L"True");
    enableDamperEffect = (targetDamperEnabled == L"true" || targetDamperEnabled == L"True");
    enableSpringEffect = (targetSpringEnabled == L"true" || targetSpringEnabled == L"True");

    // Create FFB effects as needed
    if (enableConstantForce) CreateConstantForceEffect(matchedDevice);
    if (enableDamperEffect)  CreateDamperEffect(matchedDevice);
    if (enableSpringEffect)  CreateSpringEffect(matchedDevice);

    // This is to control the max % for any of the FFB effects as specified in the ffb.ini
    // Prevents broken wrists (hopefully)
    double masterForceValue = std::stod(targetForceSetting);

    // Start telemetry processing!
    std::thread processThread(ProcessLoop);
    processThread.detach();

    // Now that we're doing everything we can display stuff!
    // Main Display Loop - Set to 200ms? Probably fine
    // Flickers a lot right now but perhaps moving to a GUI will solve that eventually
    while (true) {

        // make sure we stay at most recent display update
        MoveCursorToLine(0);

        //Trigger display
        {
            std::lock_guard<std::mutex> lock(displayMutex);
            DisplayTelemetry(displayData, masterForceValue);
        }

     /* {
            //Telemetry display for live app

            std::lock_guard<std::mutex> lock(displayMutex);
            std::cout << std::fixed << std::setprecision(2);

            std::wcout << L"ICR2 FFB Program Version 0.4 BETA\n"; //keep version up to date
            std::wcout << L"USE AT YOUR OWN RISK\n";
            std::wcout << L"Connected Device: " << targetDeviceName << L"\n";
            std::cout << "Master Force Scale: " << masterForceValue << "%\n\n";
            std::cout << "      == Raw Data ==\n" << std::endl;
            std::cout << "dLat: " << displayData.dlat << "   dLong: " << displayData.dlong << "\n";
            std::cout << "Centerline Rotation: " << displayData.rotation_deg << " deg\n";
            std::cout << "Speed: " << displayData.speed_mph << " mph\n";
            std::cout << "Steering Raw: " << displayData.steering_raw << "\n";
            std::cout << "Steering Lock Degree: " << displayData.steering_deg << "\n\n";

            std::cout << "      == Tire Loads ==\n" << std::endl;
            std::cout << "Front Left" << "      " << "Front Right\n";
            std::cout << displayData.tireload_lf << "           " << displayData.tireload_rf << "\n\n";
            // std::cout << displayData.tiremaglat_lf << "           " << displayData.tiremaglat_rf << "\n\n"; //<- some odd tire param

            //Disabled other tire values, I find its nicer to see the raw data
            //std::cout << displayData.slipNorm_lf << "           " << displayData.slipNorm_rf << "\n\n";
            //std::cout << displayData.slipMag_lf << "           " << displayData.slipMag_rf << "\n\n";

            std::cout << "Rear Left" << "       " << "Rear Right\n";
            std::cout << displayData.tireload_lr << "           " << displayData.tireload_rr << "\n\n";
           // std::cout << displayData.tiremaglat_lr << "           " << displayData.tiremaglat_rr << "\n\n"; //<- some odd tire param

            //std::cout << displayData.slipNorm_lr << "           " << displayData.slipNorm_rr << "\n\n";
            //std::cout << displayData.slipMag_lr << "           " << displayData.slipMag_rr << "\n\n";

            std::cout << "      == Calculated Data (probably wrong) ==\n" << std::endl;
            std::cout << "Direction Value: " << displayData.directionVal << "\n";
            std::cout << "Slip: " << displayData.slipAngleDeg << "\n";
            std::cout << "Lateral G: " << displayData.lateralG << " G\n";
            std::cout << "Force Magnitude: " << displayData.forceMagnitude << "\n";
            std::cout << "----------------------------------------\n";

            std::wcout << L"Log:\n";
        }
        */
        //Print log data
        {
            std::lock_guard<std::mutex> lock(logMutex);
            int maxDisplayLines = 1; //how many lines to display
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

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return 0;
}
