#pragma once

#define STRINGIFY_W(x)           L##x
#define STRINGIFY_WIDE(x)        STRINGIFY_W(#x)
#define STRINGIFY_WIDE_STRING(x) L"" STRINGIFY_W(x) L""

// delete can be called on nullptr
#define SAFE_DELETE(x) \
    delete (x);        \
    (x) = NULL
