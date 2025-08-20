#pragma once

#include "project_dependencies.h"

enum GameVersion
#if defined(IS_CPP11_COMPLIANT)
    : unsigned char
#endif
{
    VERSION_UNINITIALIZED,
    ICR2_DOS4G_1_02,
    ICR2_RENDITION
};
