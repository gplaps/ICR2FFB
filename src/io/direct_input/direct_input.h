#pragma once

#include "project_dependencies.h"
#include "ffb_device.h"

#include <vector>

class DirectInput;
// internal represenation, with all the info returned from DirectInput API and runtime instance if created
struct DirectInputDevice
{
    explicit DirectInputDevice(const DIDEVICEINSTANCE& instanceIn) :
        instance(instanceIn),
        runtime(NULL) {}
    DIDEVICEINSTANCE    instance;
    IDirectInputDevice8* runtime;
};

class EnumDeviceHelper
{
public:
    explicit EnumDeviceHelper(DirectInput* d) :
        di(d) {}
    DirectInput*                    di;
    std::vector<DirectInputDevice>& AccessKnownDevices();
};

class DirectInput
{
public:
    IDirectInputDevice8* InitializeDevice(const std::wstring& productNameOrIndex);

    // List of device productNames, indices of returned vector can be used in InitializeDevice calls
    std::vector<std::wstring> AvailableDevices() const;

    bool Valid() const;

    void LogDeviceList() const; // maybe move out

    // singleton
    static DirectInput* Instance();
    static bool         CloseInstance(); // only call this on program exit!

private:
    // private so only the Instance() method can create a "singleton" on first access
    DirectInput();
    ~DirectInput();

    bool Initialize();
    void UpdateAvailableDevice();

    std::vector<DirectInputDevice> knownDevices;
    friend class EnumDeviceHelper; // only accessor to knownDevices to fill the vector

    IDirectInputDevice8* CreateDevice(const std::wstring& productName);
    IDirectInputDevice8* CreateDevice(size_t index);
    IDirectInputDevice8* CreateDeviceInternal(DirectInputDevice* device);
    DirectInputDevice*   GetDevice(const std::wstring& productName);
    DirectInputDevice*   GetDevice(size_t index);

    LPDIRECTINPUT8      directInputObject; // consider making it static
    static DirectInput* singleton;         // as there is only one instance across the program

    bool mInitialized;
};
