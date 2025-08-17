#pragma once
#include "project_dependencies.h"

#include "ffb_device.h"

// could be a namespace as well
struct DirectInput
{
    static LPDIRECTINPUT8 directInput;

    static void ListAvailableDevices();
    static void ShowAvailableDevicesOnConsole();
    // Kick-off DirectInput
    static bool InitializeDevice(FFBDevice& device);
};
