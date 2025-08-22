// FFB for ICR2
// I don't know what I am doing!


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
#include "log.h"
#include "telemetry_display.h"
#include "timing.h"
#include "utilities.h"
#include "window.h"

// === Shared Globals ===
#if defined(HAS_STL_THREAD_MUTEX)
#    include <atomic>
#    include <thread>
std::atomic<bool> shouldExit = {};
#else
bool shouldExit = false;
#endif

// the aim is to not have global structs as they need global constructor where the initialization order is undefined.
// -> manual resource management is needed - be careful about possible resource leaks! - thats where C++ matured with patterns like RAII and shared_ptrs, but there are too many differeces between C++98 and later to cover everything, which would result in yet another split implementation (e.g. auto_ptr vs shared_ptr)
static FFBProcessor*    ffbProcessor = NULL;
static Timing*          timing       = NULL;
static const FFBConfig* config       = NULL;

static DWORD WINAPI ProcessLoop(LPVOID /*lpThreadParameter*/)
{
    // Loop which kicks stuff off and coordinates everything!
    while (!shouldExit)
    {
        if (timing->ffb.ready())
        {
            ffbProcessor->Update();
            timing->ffb.schedule();
        }
    }
    return 0;
}

static DWORD WINAPI RenderLoop(LPVOID /*plThreadParameter*/)
{
    TelemetryDisplay display;
    // Now that we're doing everything we can display stuff!
    // Main Display Loop - Set to 200ms? Probably fine
    // Flickers a lot right now but perhaps moving to a GUI will solve that eventually
    while (!shouldExit)
    {
        if (timing->render.ready())
        {
            display.Update(*config, ffbProcessor->DisplayData());
            PrintToLogFile();
            timing->render.schedule();
        }
    }
    return 0;
}

static void CloseCommon()
{
    SAFE_DELETE(ffbProcessor);
    SAFE_DELETE(timing);
    SAFE_DELETE(config);
    SAFE_DELETE(logger);
}

static void CloseMutexes()
{
#if !defined(HAS_STL_THREAD_MUTEX)
    SAFE_DELETE(Logger::mutex);
    SAFE_DELETE(TelemetryDisplay::mutex);
#endif
}

#define ENSURE(x)       \
    if (!(x))           \
    {                   \
        CloseCommon();  \
        CloseMutexes(); \
        return -1;      \
    }                   \
    do {                \
    } while (0)

// Where it all happens
int main()
{
    int res = 0;
    STATUS_CHECK(CheckAndRestartAsAdmin());

#if !defined(HAS_STL_THREAD_MUTEX)
    Logger::mutex = new CRITICAL_SECTION;
    ENSURE(Logger::mutex);
    InitializeCriticalSection(Logger::mutex);
    TelemetryDisplay::mutex = new CRITICAL_SECTION;
    ENSURE(TelemetryDisplay::mutex);
    InitializeCriticalSection(TelemetryDisplay::mutex);
#endif

    logger = new Logger("log.txt");
    ENSURE(logger);

    ENSURE(!InitConsole());
    config = new FFBConfig;
    ENSURE(config && config->Valid());
    timing = new Timing;
    ENSURE(timing);
    ffbProcessor = new FFBProcessor(*config);
    ENSURE(ffbProcessor && ffbProcessor->Valid());

    // Start telemetry processing!
#if defined(HAS_STL_THREAD_MUTEX)
    std::thread processThread([]() { ProcessLoop(NULL); });
    std::thread renderThread([]() { RenderLoop(NULL); });
    processThread.join();
    renderThread.join();
    CloseCommon();
#else
    DWORD  threadID   = 0;
    HANDLE threads[2] = {};

    threads[0]        = CreateThread(NULL, 0, ProcessLoop, NULL, 0, &threadID);
    ENSURE(threads[0]);
    threads[1] = CreateThread(NULL, 0, RenderLoop, NULL, 0, &threadID);
    ENSURE(threads[1]);
    WaitForMultipleObjects(2, threads, TRUE, INFINITE); // std::thread::join()
    CloseHandle(threads[0]);
    CloseHandle(threads[1]);

    CloseCommon();

    DeleteCriticalSection(Logger::mutex);
    DeleteCriticalSection(TelemetryDisplay::mutex);
    CloseMutexes();
#endif

    return 0;
}
