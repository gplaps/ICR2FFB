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
#include "ffb_processor.h"
#include "helpers.h"
#include "log.h"
#include "telemetry_display.h"
#include "window.h"

// === Standard Library Includes ===
#include <fstream>
#include <iostream>

// === Shared Globals ===
#if defined(HAS_STL_THREAD_MUTEX)
#    include <atomic>
#    include <thread>
std::atomic<bool> shouldExit = {};
#else
bool shouldExit = false;
#endif

#if !defined(HAS_STL_THREAD_MUTEX)
static FFBProcessor* ffbProcessor = NULL;

static DWORD WINAPI ProcessLoop(LPVOID /*lpThreadParameter*/)
{
    // Loop which kicks stuff off and coordinates everything!
    while (!shouldExit)
    {
        ffbProcessor->Update();
        Sleep(16);
    }
    return 0;
}
#endif

// Where it all happens
int main()
{
#if !defined(HAS_STL_THREAD_MUTEX)
    Logger::mutex = CreateMutex(NULL, FALSE, NULL);
    if (Logger::mutex == NULL)
    {
        LogMessage(L"[ERROR] Failed to create log mutex");
        return -1;
    }
    TelemetryDisplay::mutex = CreateMutex(NULL, FALSE, NULL);
    if (TelemetryDisplay::mutex == NULL)
    {
        LogMessage(L"[ERROR] Failed to create display mutex");
        return -1;
    }
#endif

    if (!logger) { logger = new Logger("log.txt"); }

    int res = 0;
    STATUS_CHECK(CheckAndRestartAsAdmin());
    STATUS_CHECK(InitConsole());

    const FFBConfig config;
    if (!config.Valid())
    {
        return -1;
    }

    // Start telemetry processing!
#if defined(HAS_STL_THREAD_MUTEX)
    FFBProcessor ffbProcessor(config);
    if (!ffbProcessor.Valid())
    {
        return -1;
    }

    std::thread processThread([&]() {
        // Loop which kicks stuff off and coordinates everything!
        while (!shouldExit)
        {
            ffbProcessor.Update();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    });
    processThread.detach();
#else
    ffbProcessor = new FFBProcessor(config);
    if (!ffbProcessor || !ffbProcessor->Valid())
    {
        return -1;
    }

    DWORD  threadID = 0;
    HANDLE hThread  = CreateThread(NULL, 0, ProcessLoop, NULL, 0, &threadID);
    if (hThread == NULL)
    {
        LogMessage(L"[ERROR] Failed to create thread.");
        return -1;
    }
#endif

    TelemetryDisplay display;
    // Now that we're doing everything we can display stuff!
    // Main Display Loop - Set to 200ms? Probably fine
    // Flickers a lot right now but perhaps moving to a GUI will solve that eventually
    while (!shouldExit)
    {
#if defined(HAS_STL_THREAD_MUTEX)
        display.Update(config, ffbProcessor.DisplayData());
        PrintToLogFile();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
#else
        display.Update(config, ffbProcessor->DisplayData());
        PrintToLogFile();
        Sleep(100);
#endif
    }

#if !defined(HAS_STL_THREAD_MUTEX)
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
    delete ffbProcessor;
    CloseHandle(Logger::mutex);
    CloseHandle(TelemetryDisplay::mutex);
#endif

    delete logger; // destructs logger, flushes and closes file

    return 0;
}
