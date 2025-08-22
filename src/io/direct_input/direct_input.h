#pragma once

#include "ffb_device.h"

#include <vector>

// could be a namespace as well
struct DirectInput
{
    DirectInput();
    ~DirectInput();

    bool InitializeDevice(FFBDevice& device);

    // List of device productNames, indices of returned vector can be used in CreateDevice calls
    std::vector<std::wstring> AvailableDevices() const;
    struct DirectInputDevice
    {
        explicit DirectInputDevice(const DIDEVICEINSTANCEW& instanceIn) :
            instance(instanceIn),
            runtime(NULL) {}
        DIDEVICEINSTANCEW    instance;
        IDirectInputDevice8* runtime;
    };
    std::vector<DirectInputDevice> knownDevices;

    bool Valid() const;

    // ProductNames and indices can be obtained from AvailableDevices()
    FFBDevice* CreateDevice(const std::wstring& productName);
    FFBDevice* CreateDevice(size_t index);

    void LogDeviceList() const; // maybe move out

private:
    bool Initialize();
    void UpdateAvailableDevice();

    // internal represenation, closer to DirectInput API
    DirectInputDevice* GetDevice(const std::wstring& productName);
    DirectInputDevice* GetDevice(size_t index);
    FFBDevice*         CreateDeviceInternal(DirectInputDevice* device);


    LPDIRECTINPUT8 directInputObject; // static

    bool mInitialized;
};

extern DirectInput* directInput;
