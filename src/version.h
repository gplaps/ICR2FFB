#pragma once

#define STRINGIFY_W(x)           L##x
#define STRINGIFY_WIDE(x)        STRINGIFY_W(#x)
#define STRINGIFY_WIDE_STRING(x) L"" STRINGIFY_W(x) L""

#define VERSION_NUMBER           STRINGIFY_WIDE_STRING("0.9.0 BETA")
#define VERSION_STRING           L"ICR2 FFB Program Version " VERSION_NUMBER
