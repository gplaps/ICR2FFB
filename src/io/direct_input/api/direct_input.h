#pragma once

#include <dinput.h>

#include <string>
#include <vector>

class DirectInput;
// internal representation, with all the info returned from DirectInput API and runtime instance if created
struct DiDeviceData
{
    explicit DiDeviceData(const DIDEVICEINSTANCE& instanceIn) :
        instance(instanceIn),
        runtime(NULL) {}
    DIDEVICEINSTANCE     instance;
    IDirectInputDevice8* runtime;
};

class EnumDeviceHelper
{
public:
    explicit EnumDeviceHelper(DirectInput* d) :
        di(d) {}
    DirectInput*               di;
    std::vector<DiDeviceData>& AccessKnownDevices() const;
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

    std::vector<DiDeviceData> knownDevices;
    friend class EnumDeviceHelper; // only accessor to knownDevices to fill the vector

    IDirectInputDevice8* CreateDevice(const std::wstring& productName);
    IDirectInputDevice8* CreateDevice(size_t index);
    IDirectInputDevice8* CreateDeviceInternal(DiDeviceData* device);
    DiDeviceData*        GetDevice(const std::wstring& productName);
    DiDeviceData*        GetDevice(size_t index);

    LPDIRECTINPUT8      directInputObject; // thats why there should only be one instance of DirectInput in the program
    static DirectInput* singleton;         // ensures only one instance across the program

    bool mInitialized;
};
