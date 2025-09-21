#pragma once

#include "ffb_device.h"

// not implemented, just a preparation for possible other device types
struct FFBPedals : public FFBDevice
{
    explicit FFBPedals(const FFBConfig& config);
    ~FFBPedals() OVERRIDE;
};
