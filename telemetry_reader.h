// telemetry_reader.h
#include <string>
#pragma once

struct RawTelemetry {
    double dlat;
    double dlong;
    double rotation_deg;
    double speed_mph;
    double steering_deg;
    double steering_raw;
    double tireload_lf;
    double tireload_rf;
    double tireload_lr;
    double tireload_rr;
    double tiremaglat_lf;
    double tiremaglat_rf;
    double tiremaglat_lr;
    double tiremaglat_rr;
    bool valid = false;
};

// Include logging
void LogMessage(const std::wstring& msg);

// Returns true if data was read successfully
bool ReadTelemetryData(RawTelemetry& out);