#include "telemetry_display.h"

#include "version.h"
#include "window.h"

#include <iomanip>
#include <iostream>
#include <sstream>

DEFINE_MUTEX(TelemetryDisplay::mutex);
// Define console width
const unsigned int CONSOLE_WIDTH = 80;

// Helper function to pad lines
static std::wstring padLine(const std::wstring& text)
{
    std::wstring padded = text;
    if (padded.length() < CONSOLE_WIDTH)
    {
        padded.append(CONSOLE_WIDTH - padded.length(), L' ');
    }
    else if (padded.length() > CONSOLE_WIDTH)
    {
        padded.resize(CONSOLE_WIDTH); // Truncate if too long
    }
    return padded;
}

// New display
void TelemetryDisplay::DisplayTelemetry(const FFBConfig& config) const
{
    // make sure we stay at most recent display update
    MoveCursorToLine(0);
    // Move cursor to top and set up formatting
    MoveCursorToTop();

    std::cout << std::fixed << std::setprecision(2);
    std::wcout << std::fixed << std::setprecision(2); // Also set for wide cout

    // Header section
    std::wcout << padLine(VERSION_STRING) << L"\n";
    std::wcout << padLine(L"USE AT YOUR OWN RISK") << L"\n";
    std::wcout << padLine(L"Connected Device: " + config.GetString(L"base", L"device")) << L"\n";

    std::wostringstream ss;
    ss << std::fixed << std::setprecision(2); // Set formatting for stringstream too
    ss << L"Master Force Scale: " << displayData.masterForceScale * 100.0 << L"%";
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n"; // Empty line

    // Raw data section
    std::wcout << padLine(L"      == Raw Data ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L"");
    ss.clear();
    ss << L"dLat: " << std::setw(10) << displayData.raw.pos.dlat << L"   dLong: " << std::setw(10) << displayData.raw.pos.dlong;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L"");
    ss.clear();
    ss << L"Centerline Rotation: " << std::setw(8) << displayData.raw.pos.rotation_deg << L" deg";
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L"");
    ss.clear();
    ss << L"Speed: " << std::setw(8) << displayData.raw.speed_mph << L" mph";
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L"");
    ss.clear();
    ss << L"Steering Raw: " << std::setw(10) << displayData.raw.steering_raw;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L"");
    ss.clear();
    ss << L"Steering Lock Degree: " << std::setw(8) << displayData.raw.steering_deg;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    // Tire loads section
    std::wcout << padLine(L"      == Tire Loads ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";
    std::wcout << padLine(L"Front Left      Front Right") << L"\n";

    ss.str(L"");
    ss.clear();
    ss << std::setw(10) << L"long: " << static_cast<int16_t>(displayData.raw.tiremaglong_lf) << L"           " << std::setw(10) << static_cast<int16_t>(displayData.raw.tiremaglong_rf);
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L"");
    ss.clear();
    ss << std::setw(10) << L"lat: " << static_cast<int16_t>(displayData.raw.tiremaglat_lf) << L"           " << std::setw(10) << static_cast<int16_t>(displayData.raw.tiremaglat_rf);
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    std::wcout << padLine(L"Rear Left       Rear Right") << L"\n";
    //ss.str(L""); ss.clear();
    //ss << std::setw(10) << displayData.raw.tireload_lr << L"           " << std::setw(10) << displayData.tireload_rr;
    //std::wcout << padLine(ss.str()) << L"\n";
    //std::wcout << padLine(L"") << L"\n";

    ss.str(L"");
    ss.clear();
    ss << std::setw(10) << L"long: " << static_cast<int16_t>(displayData.raw.tiremaglong_lr) << L"           " << std::setw(10) << static_cast<int16_t>(displayData.raw.tiremaglong_rr);
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L"");
    ss.clear();
    ss << std::setw(10) << L"lat: " << static_cast<int16_t>(displayData.raw.tiremaglat_lr) << L"           " << std::setw(10) << static_cast<int16_t>(displayData.raw.tiremaglat_rr);
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    // Vehicle Dynamics section
    std::wcout << padLine(L"      == Vehicle Dynamics ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L"");
    ss.clear();
    ss << L"Lateral G: " << std::setw(8) << displayData.vehicleDynamics.lateralG << L" G";
    std::wcout << padLine(ss.str()) << L"\n";

    //ss.str(L""); ss.clear();
    //ss << L"Yaw Rate: " << std::setw(8) << displayData.vehicleDynamics.yaw << L" deg/s�";
    //std::wcout << padLine(ss.str()) << L"\n";

    //ss.str(L"");
    //ss.clear();
    //ss << L"Longi Force: " << std::setw(8) << displayData.long_force << L"";
    //std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L"");
    ss.clear();
    ss << L"Direction Value: " << displayData.vehicleDynamics.directionVal;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L"");
    ss.clear();
    ss << L"Force Magnitude: " << displayData.constantForce.magnitude;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    /*
    // Tire Forces
    std::wcout << padLine(L"      == Decoded Tire Forces ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";
    std::wcout << padLine(L"Front Left      Front Right") << L"\n";

    ss.str(L""); ss.clear();
    ss << std::setw(10) << displayData.vehicleDynamics.force_lf << L"           " << std::setw(10) << displayData.vehicleDynamics.force_rf;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    std::wcout << padLine(L"Rear Left       Rear Right") << L"\n";
    ss.str(L""); ss.clear();
    ss << std::setw(10) << displayData.vehicleDynamics.force_lr << L"           " << std::setw(10) << displayData.vehicleDynamics.force_rr;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Front Total: " << std::setw(8) << displayData.vehicleDynamics.frontLateralForce << L"   Rear Total: " << std::setw(8) << displayData.vehicleDynamics.rearLateralForce;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Total Force: " << std::setw(8) << displayData.vehicleDynamics.totalLateralForce << L"   Yaw Moment: " << std::setw(8) << displayData.vehicleDynamics.yawMoment;
    std::wcout << padLine(ss.str()) << L"\n";
    std::wcout << padLine(L"") << L"\n";
    */

    /*
    // Legacy calculated data section
    std::wcout << padLine(L"      == Legacy Calculated Data ==") << L"\n";
    std::wcout << padLine(L"") << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Direction Value: " << displayData.vehicleDynamics.directionVal;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Slip: " << displayData.slip.slipAngleDeg;
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Lateral G: " << displayData.vehicleDynamics.lateralG << L" G";
    std::wcout << padLine(ss.str()) << L"\n";

    ss.str(L""); ss.clear();
    ss << L"Force Magnitude: " << static_cast<int>(displayData.slip.forceMagnitude * (double)DEFAULT_DINPUT_GAIN);
    std::wcout << padLine(ss.str()) << L"\n";

    */
    std::wcout << padLine(L"----------------------------------------") << L"\n";
    std::wcout << padLine(L"Log:") << L"\n";
}

void TelemetryDisplay::Update(const FFBConfig& config, const TelemetryDisplayData& displayDataIn)
{
    {
        LOCK_MUTEX(mutex);
        displayData = displayDataIn;
        UNLOCK_MUTEX(mutex);
    }
    //Trigger display
    DisplayTelemetry(config);
}
