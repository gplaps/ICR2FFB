#pragma once

#define STRINGIFY_W(x)           L##x
#define STRINGIFY_WIDE_STRING(x) L"" STRINGIFY_W(x) L""
#if defined(UNICODE)
#    define STRINGIFY_STRING(x) L##x
#    define STRINGIFY(x)        L"" STRINGIFY_STRING(x) L""
#else
#    define STRINGIFY_STRING(x) #x
#    define STRINGIFY(x)        "" STRINGIFY_STRING(x) ""
#endif
// delete can be called on nullptr
#define SAFE_DELETE(x) \
    delete (x);        \
    (x) = NULL
