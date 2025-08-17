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
 
#include "main.h"

// === Project Includes ===
#include "project_dependencies.h"
#include "constants.h"
#include "helpers.h"
#include "direct_input.h"
#include "telemetry_reader.h"
#include "telemetry_display.h"
#include "ffb_processor.h"
#include "log.h"
#include "window.h"

// === Standard Library Includes ===
#include <fstream>
#include <iostream>
#include <thread>
#include <atomic>
#include <string>


// === Shared Globals ===
std::atomic<bool> shouldExit = {};

// Where it all happens
int main() {
    int res;
    STATUS_CHECK(CheckAndRestartAsAdmin());
    STATUS_CHECK(InitConsole());

    //clear last log
    std::wofstream clearLog("log.txt", std::ios::trunc);

    const FFBConfig config;
    if(!config.Valid())
        return -1;

    FFBProcessor ffbProcessor(config);
    if(!ffbProcessor.Valid())
        return -1;

    // Start telemetry processing!
    std::thread processThread([&]() {
        // Loop which kicks stuff off and coordinates everything!
        while (!shouldExit) {
            ffbProcessor.Update();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });
    processThread.detach();

    TelemetryDisplay display;
    // Now that we're doing everything we can display stuff!
    // Main Display Loop - Set to 200ms? Probably fine
    // Flickers a lot right now but perhaps moving to a GUI will solve that eventually
    while (!shouldExit) {
        display.Update(config, ffbProcessor.DisplayData());
        PrintToLogFile();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return 0;
}
