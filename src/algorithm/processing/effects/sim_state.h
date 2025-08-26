#pragma once
#include "project_dependencies.h"

// unused, but can maybe be derived or read directly
enum SimState
#if defined(IS_CPP11_COMPLIANT)
    : unsigned char
#endif
{
    ST_UNKNOWN,
    ST_STOPPED,
    ST_PAUSED,
    ST_RUNNING,
    ST_REPLAY
};
