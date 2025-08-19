#pragma once

// a function-like macro to check return code like FAILED/SUCCESS for HRESULT
#define STATUS_CHECK(func) \
    res = (func);          \
    if (res) return res

#define SAFE_DELETE(x) \
    delete (x);        \
    (x) = NULL
