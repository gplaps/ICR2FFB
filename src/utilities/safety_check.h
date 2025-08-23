#pragma once
#include "log.h"
#include "string_utilities.h"

#include <cmath>

#define SAFETY_CHECK(x)                                                                              \
    if (!std::isfinite((x)))                                                                         \
    {                                                                                                \
        LogMessage(AnsiToWide(__FILE__) + L":" + std::to_wstring(__LINE__) + L" - NaN encountered"); \
        x = 0;                                                                                       \
    }                                                                                                \
    do {                                                                                             \
    } while (0)
