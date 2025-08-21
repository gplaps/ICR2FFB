#pragma once

#define STRINGIFY_W(x)           L##x
#define STRINGIFY_WIDE(x)        STRINGIFY_W(#x)
#define STRINGIFY_WIDE_STRING(x) L"" STRINGIFY_W(x) L""

// a function-like macro to check return code like FAILED/SUCCESS for HRESULT
#define STATUS_CHECK(func) \
    res = (func);          \
    if (res) return res

// delete can be called on nullptr
#define SAFE_DELETE(x) \
    delete (x);        \
    (x) = NULL
