#pragma once
#include "project_dependencies.h" // IWYU pragma: keep

#include "ffb_config.h"
#include "game_version.h"
#include "memoryapi.h"

#if !defined(IS_CPP11_COMPLIANT)
#    include <stdint.h>
#endif

struct WorldPos
{
    double dlat;
    double dlong;
    double rotation_deg;
};

struct RawTelemetry
{
    WorldPos pos;

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

    double tiremaglong_lf;
    double tiremaglong_rf;
    double tiremaglong_lr;
    double tiremaglong_rr;

    bool valid;
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
    bool Initialize(const FFBConfig& config);
    void ConvertCarData();
    void ConvertTireData();
    bool ReadCarData();
    bool ReadTireData();
    template <typename T>
    bool ReadValue(T& dest, uintptr_t src)
    {
        SIZE_T bytesRead = 0;
        return ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(src), &dest, sizeof(dest), &bytesRead) && bytesRead == sizeof(dest);
    }
    bool ReadRaw(void* dest, uintptr_t src, SIZE_T size);

    struct RawData
    {
        int16_t loadLF;
        int16_t loadRF;
        int16_t loadLR;
        int16_t loadRR;

        int16_t magLatLF;
        int16_t magLatRF;
        int16_t magLatLR;
        int16_t magLatRR;

        int16_t magLongLF;
        int16_t magLongRF;
        int16_t magLongLR;
        int16_t magLongRR;
    };

    struct CarData
    {
#if defined(IS_CPP11_COMPLIANT)
        int32_t data[12] = {0};
#else
        int32_t data[12];
#endif
    };

    HANDLE       hProcess;
    bool         mInitialized;
    GameOffsets  offsets;
    RawTelemetry out;
    RawData      rawData;
    CarData      carData;
};
