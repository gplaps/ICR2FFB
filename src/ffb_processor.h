#pragma once

#include "direct_input.h"
#include "ffb_config.h"
#include "ffb_output.h"
#include "telemetry_reader.h"
#include "telemetry_display.h"

struct FFBProcessor {
    FFBProcessor(const FFBConfig& config);
    bool Valid();
    void Update();
    const TelemetryDisplay::TelemetryDisplayData& DisplayData() const;

private:
    //Get some data from RawTelemetry -> not 100% sure what this does
    RawTelemetry current{};
    RawTelemetry previousLat{};
    RawTelemetry previousSlip{};
    RawTelemetry previousVD{};
    bool firstReadingLat = true;
    bool firstReadingSlip = true;
    bool firstReadingVD = true;

    double previousDlong = 0.0;
    int noMovementFrames = 0;
    const int movementThreshold = 3;  // number of frames to consider "stopped"
    bool effectPaused = false;

    //Added for feedback skipping if stopped
    RawTelemetry previousPos{};
    bool firstPos = true;

    TelemetryReader telemetryReader;
    FFBOutput ffbOutput;
    TelemetryDisplay::TelemetryDisplayData displayData;
    bool mInitialized = false;
};
