#pragma once
#include "project_dependencies.h"

#include "ffb_config.h"
#include "memoryapi.h"

#include <cstdint>
#include <string>

struct RawTelemetry
{
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
    double long_force;
    bool   valid;
};

// Things to look for in the Memory to make it tick
struct GameOffsets
{
    uintptr_t signatureOffset;
    uintptr_t cars_data_offset;
    uintptr_t tire_data_offsetfl;
    uintptr_t tire_data_offsetfr;
    uintptr_t tire_data_offsetrl;
    uintptr_t tire_data_offsetrr;
    uintptr_t tire_maglat_offsetfl;
    uintptr_t tire_maglat_offsetfr;
    uintptr_t tire_maglat_offsetrl;
    uintptr_t tire_maglat_offsetrr;
    uintptr_t car_longitude_offset;

    void ApplySignature(uintptr_t sigAddr);
};

struct TelemetryReader
{
public:
    explicit TelemetryReader(const FFBConfig& config);
    ~TelemetryReader();

    bool                Update();
    bool                Initialized() const;
    bool                Valid() const;
    const RawTelemetry& Data() const;

private:
    struct CarData
    {
        int32_t data[12]
#if !(defined(_MSC_VER) && _MSC_VER <= 1800) // MSVC 2013
        = {0}
#endif
        ;
    } carData;

    void ConvertCarData();
    void ConvertTireData();
    bool ReadLongitudinalForce();
    bool ReadCarData();
    bool ReadTireData();
    template <typename T>
    bool ReadValue(T& dest, uintptr_t offset)
    {
        SIZE_T bytesRead = 0;
        return ReadProcessMemory(hProcess, (LPCVOID)offset, &dest, sizeof(dest), &bytesRead) && bytesRead == sizeof(dest);
    }
    bool ReadRaw(void* dest, uintptr_t offset, SIZE_T size);

    struct RawData
    {
        int16_t loadLF   = 0;
        int16_t loadRF   = 0;
        int16_t loadLR   = 0;
        int16_t loadRR   = 0;
        int16_t magLatLF = 0;
        int16_t magLatRF = 0;
        int16_t magLatLR = 0;
        int16_t magLatRR = 0;
        int16_t longiF   = 0;
    };

    HANDLE       hProcess;
    bool         mInitialized;
    GameOffsets  offs;
    RawTelemetry out;
    RawData      rawData;
};
