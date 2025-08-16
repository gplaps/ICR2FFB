#pragma once
#include "project_dependencies.h"
#include "ffb_device.h"
#include <string>

struct SpringEffect {
    void Update(FFBDevice& device, double masterForceScale);
};
